#!/usr/bin/python
from __future__ import print_function
import fcntl, hashlib, json, os, re, signal, subprocess, sys, threading, time

BASE = '/tmp/lg-linux-tv'
FLAG = '/sys/kernel/kexec_loaded'
sys.path.insert(0, BASE)
import monitor_state as monitors


def read(path):
    with open(path) as handle:
        return handle.read().strip()


def state():
    return int(read(FLAG))


def bounded(command, log, seconds):
    process = subprocess.Popen(command, stdin=open('/dev/null'), stdout=log,
                               stderr=subprocess.STDOUT, close_fds=True)
    timed_out = []
    def stop():
        if process.poll() is None:
            timed_out.append(True)
            process.kill()
    timer = threading.Timer(seconds, stop); timer.daemon = True; timer.start()
    try:
        code = process.wait()
    finally:
        timer.cancel()
    if timed_out:
        raise RuntimeError('Command timeout')
    return code


def verify():
    with open(BASE + '/stage-manifest.json') as handle:
        manifest = json.load(handle)
    for name, expected in manifest['files'].items():
        if not re.match(r'^[A-Za-z0-9_.-]+$', name):
            raise ValueError('Invalid filename')
        with open(BASE + '/' + name, 'rb') as handle:
            actual = hashlib.sha256(handle.read()).hexdigest()
        if actual != expected:
            raise ValueError('Checksum mismatch: ' + name)
    if os.uname()[2] != manifest['stock_kernel']:
        raise ValueError('Unexpected stock kernel: ' + os.uname()[2])
    if 'rtd288o' not in read('/proc/cpuinfo'):
        raise ValueError('Unexpected platform')
    banks = []
    for line in read('/proc/iomem').splitlines():
        match = re.match(r'^([0-9a-f]+)-([0-9a-f]+) : System RAM$', line)
        if match:
            banks.append([int(match.group(1), 16), int(match.group(2), 16) + 1])
    if banks != manifest['ram_banks']:
        raise ValueError('RAM map differs from the tested device')
    return manifest


def command(manifest):
    return [BASE + '/kexec-arm', '-l', BASE + '/uImage-rtd288o', '--type=uImage',
            '--image-size=' + manifest['loader_image_size'],
            '--dtb=' + BASE + '/rtd288o.dtb',
            '--initrd=' + BASE + '/alpine-loader.cpio.gz',
            '--command-line=' + manifest['command_line'],
            '--mem-min=0x100000', '--mem-max=0x15ffffff']


def offline_secondary_cpus():
    changed = []
    try:
        for cpu in (3, 2, 1):
            path = '/sys/devices/system/cpu/cpu%d/online' % cpu
            if os.path.exists(path) and read(path) == '1':
                with open(path, 'w') as handle:
                    handle.write('0\n')
                if read(path) != '0':
                    raise RuntimeError('Failed to offline CPU%d' % cpu)
                changed.append(cpu)
        online = read('/sys/devices/system/cpu/online')
        if online != '0':
            raise RuntimeError('Unexpected online CPU set: ' + online)
        return changed
    except Exception:
        restore_secondary_cpus(changed)
        raise


def restore_secondary_cpus(changed):
    for cpu in reversed(changed):
        path = '/sys/devices/system/cpu/cpu%d/online' % cpu
        with open(path, 'w') as handle:
            handle.write('1\n')


def check(manifest):
    if state() != 0:
        raise ValueError('A kernel is already staged')
    result = {'states': [0], 'success': False, 'kernel_executed': False}
    with open(BASE + '/load-unload.log', 'w') as log:
        try:
            result['load_exit'] = bounded(command(manifest), log, 20)
            result['states'].append(state())
        finally:
            result['unload_exit'] = bounded([BASE + '/kexec-arm', '-u'], log, 10)
            result['states'].append(state())
    result['success'] = (result['states'] == [0, 1, 0] and
                         result['load_exit'] == 0 and result['unload_exit'] == 0)
    with open(BASE + '/load-unload-result.json', 'w') as handle:
        json.dump(result, handle, indent=2)
    print(json.dumps(result, indent=2))
    return 0 if result['success'] else 1


def execute(manifest):
    if state() != 0 or read('/sys/class/net/eth0/carrier') != '1':
        raise ValueError('Ethernet must be connected and no kernel may be staged')
    with open(BASE + '/load-unload-result.json') as handle:
        if not json.load(handle).get('success'):
            raise ValueError('Run --check successfully before execution')
    saved = monitors.snapshot()
    offlined_cpus = []
    with open(BASE + '/monitors-before-execution.json', 'w') as handle:
        json.dump(saved, handle, indent=2)
    try:
        subprocess.check_call(command(manifest))
        if state() != 1:
            raise RuntimeError('Kernel did not stage')
        monitors.disable(saved)
        offlined_cpus = offline_secondary_cpus()
        print('Executing RAM-only Linux boot'); sys.stdout.flush()
        subprocess.check_call([BASE + '/kexec-arm', '-e'])
        raise RuntimeError('kexec unexpectedly returned')
    finally:
        for sig in (signal.SIGHUP, signal.SIGTERM, signal.SIGINT):
            signal.signal(sig, signal.SIG_IGN)
        subprocess.call([BASE + '/kexec-arm', '-u'])
        if offlined_cpus:
            restore_secondary_cpus(offlined_cpus)
        monitors.restore(saved)


def main():
    if os.geteuid() != 0:
        raise ValueError('Root required')
    if sys.argv[1:] not in (['--check'], ['--execute-once']):
        raise ValueError('Use --check or --execute-once')
    lock = open(BASE + '/load.lock', 'a')
    fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    os.chmod(BASE + '/kexec-arm', 0o755)
    manifest = verify()
    return check(manifest) if sys.argv[1] == '--check' else execute(manifest)


if __name__ == '__main__':
    try:
        sys.exit(main())
    except Exception as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
