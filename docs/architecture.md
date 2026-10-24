# Архитектура

## Почему два этапа

Полный initramfs распаковывался слишком долго. До запуска PID 1 MICOM не получал
ожидаемый запрос и перезапускал CPU. Поэтому начальный образ маленький:
freestanding PID 1 начинает MICOM-опрос сразу, а полный Alpine скачивается и
распаковывается дочерним процессом параллельно.

## Видео

Vendor GDMA нельзя безопасно инициализировать обычным ранним initcall: загрузчик
оставляет pending OSD interrupt, а исходный драйвер регистрировал IRQ раньше
создания `OsdSync` workqueue. ISR вызывал `queue_work()` с NULL и падал.

Патч создаёт workqueue перед `request_irq()`. Сам initcall остаётся в
`initcall_blacklist`; после стабилизации PID 1 модуль `gdma-trigger.ko` вызывает
экспортированный `GDMA_init_module()`, затем загружается `rtk_venusfb.ko`.

Xorg fbdev требует sysfs-ссылку `fb0/device/subsystem`, которой у виртуального
vendor framebuffer нет. `start-lg-desktop` создаёт временное представление через
bind mount. Драйвер также нормализует `fb_var_screeninfo.sync`; небольшой
`LD_PRELOAD` shim сохраняет запрошенную структуру после успешного ioctl.

## Хранилище

Текущая цепочка не монтирует stock eMMC и не устанавливает постоянный boot hook.
Полный rootfs живёт в RAM. Это ограничивает объём пакетов, но упрощает холодное
восстановление.
