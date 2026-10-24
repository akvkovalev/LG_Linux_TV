# GDMA late trigger

The vendor GDMA driver is built into the kernel but blacklisted at boot with
`initcall_blacklist=GDMA_init_module`. This tiny module calls the exported
initializer after PID 1 has established the MICOM liveness loop and networking.

The kernel patch must export `GDMA_init_module`, and `gdma-trigger.ko` must be
built against the exact kernel tree used for the boot image.
