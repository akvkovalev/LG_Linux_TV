#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import zlib

from envutil import load_env, require

repo = Path(__file__).resolve().parents[1]
env = load_env(repo / ".env")
require(env, "TV_HOST", "TV_MAC", "NETCONSOLE_HOST", "NETCONSOLE_HOST_MAC", "NETCONSOLE_PORT")
dist = Path(env.get("DIST_DIR", repo / "dist"))
bundle = dist / "boot"
bundle.mkdir(parents=True, exist_ok=True)

base_args = (
    "envp=cc100 flashtype=emmc bufsize=80000 no_console_suspend bcas=off "
    "emmc_clock=200 eBackLight=direct eLEDBarType=6 bLocalDimming=0 "
    "eModelInchType=43 eModelToolType=UN71 eModelModuleType=BOE pwm_mode=1 "
    f"ethaddr={env['TV_MAC']} rtdlog=3000 devtmpfs.mount=1 modelopt=21003004 "
    "hwopt=203200001003002000 ToolOpt=203114:16406:35584:65:60351524:273731337 "
    "debugMode=5 modelName=43UN71006LB countryGrp=22 sver=6.00.131 bver=6.00.131 "
    "chip=K6LPA0 extmod=LGTV emmc_size=0x1d2000000 root=/dev/ram0 ro rdinit=/init "
    "nr_cpus=1 maxcpus=1 panic=5 "
    "initcall_blacklist=rtk_platform_arm_wrapper_driver_init,rtk_sb2_driver_init,"
    "sb2_extra_module_init,GDMA_init_module,memc_isr_init_module,"
    "rtk_monitor_driver_init,rtk_cpufreq_driver_init "
    f"netconsole=6665@{env['TV_HOST']}/eth0,{env['NETCONSOLE_PORT']}@"
    f"{env['NETCONSOLE_HOST']}/{env['NETCONSOLE_HOST_MAC']} "
    "log_buf_len=2M console=tty0 console=ttyS0,115200 loglevel=3"
)

dts = (repo / "device/rtd288o-tested.dts").read_text()
dts, count = re.subn(r'bootargs = "[^"\n]*";', f'bootargs = "{base_args}";', dts, count=1)
if count != 1:
    raise SystemExit("DTS bootargs property not found")
dts_path = bundle / "rtd288o.dts"
dtb_path = bundle / "rtd288o.dtb"
dts_path.write_text(dts)
subprocess.run(["dtc", "-I", "dts", "-O", "dtb", "-o", dtb_path, dts_path], check=True)

raw = (dist / "Image-rtd288o").read_bytes()
fields = [0x27051956, 0, 0, len(raw), 0x108000, 0x108000, zlib.crc32(raw) & 0xFFFFFFFF]
header = struct.pack(">7I4B32s", *fields, 5, 2, 2, 0, b"LG Linux TV RAM boot")
fields[1] = zlib.crc32(header) & 0xFFFFFFFF
header = struct.pack(">7I4B32s", *fields, 5, 2, 2, 0, b"LG Linux TV RAM boot")
(bundle / "uImage-rtd288o").write_bytes(header + raw)

for source, name in ((dist / "alpine-loader.cpio.gz", "alpine-loader.cpio.gz"),
                     (dist / "kexec-arm", "kexec-arm")):
    shutil.copy2(source, bundle / name)

files = ["uImage-rtd288o", "rtd288o.dtb", "alpine-loader.cpio.gz", "kexec-arm"]
manifest = {
    "model": "LG 43UN71006LB",
    "stock_kernel": env.get("TV_STOCK_KERNEL", "4.4.84-612.40.jimna.1"),
    "loader_image_size": "0x2100000",
    "command_line": base_args,
    "ram_banks": [[0, 369098752], [385875968, 402653184],
                  [404750336, 441450496], [450887680, 455081984],
                  [476053504, 1610612736]],
    "files": {name: hashlib.sha256((bundle / name).read_bytes()).hexdigest() for name in files},
}
(bundle / "stage-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
print(json.dumps(manifest, indent=2))
