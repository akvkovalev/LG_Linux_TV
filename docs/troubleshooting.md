# Ограничения и диагностика

- Framebuffer сейчас 720×480×16; вывод программный.
- Нативный input driver пульта ещё не подключён. Xorg стартует без клавиатуры и
  мыши; управление пока выполняется по SSH/serial.
- HDMI RX, аппаратное видео и звук проверялись отдельно в webOS окружении. В
  native Alpine они ещё не оформлены как обычные Linux-драйверы.
- Rootfs загружается по HTTP при каждом запуске и должен помещаться вместе с
  распакованной системой в доступную RAM.
- Совместимость подтверждена только на 43UN71006LB/04.63.25.

Логи в native Alpine:

```sh
cat /run/log/network.log
cat /run/log/display.log
cat /run/log/desktop.log
cat /run/lg-desktop/Xorg.0.log
cat /run/log/sshd.log
```

Если есть SSH, но нет изображения:

```sh
lsmod | grep -E 'gdma_trigger|rtk_venusfb'
cat /proc/fb
ls -l /dev/fb0 /tmp/.X11-unix/X0
```
