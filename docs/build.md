# Сборка

## Зависимости хоста

Рекомендуется Debian/Ubuntu x86_64:

```sh
sudo apt install build-essential bc bison flex libssl-dev libelf-dev \
    device-tree-compiler python3 curl xz-utils qemu-user-static binfmt-support
```

Нужны также:

1. GPL-архив LG `kernel-1.0.0-612.21.tar.gz`, полученный с портала LG Open Source.
2. ARMv7 webOS cross-toolchain с префиксом `arm-webos-linux-gnueabi-`.
3. Alpine 3.22.6 ARMv7 minirootfs. Проверенный SHA-256 указан в
   `checksums/reference.sha256`.
4. Публичный SSH-ключ для `/root/.ssh/authorized_keys` в Alpine.

Скопируйте `config/example.env` в `.env` и укажите абсолютные пути. `.env`
игнорируется Git. Все SSH-команды рассчитаны на ключ, пароль сохранять не нужно.

## Ядро

`apply-kernel-patch.sh` распаковывает чистое дерево `linux-4.4.3` и применяет
`patches/0001-tested-native-boot.patch`. Это точная разница между исходным архивом
612.21 и деревом успешного образа. Патч содержит:

- совместимость старого дерева с современными host GCC/binutils;
- диагностические breadcrumb-маркеры ранней загрузки;
- отключение неприменимых watchdog-проверок DSP;
- изоляцию vendor file logger для RAM-only системы;
- исправление порядка GDMA workqueue/IRQ;
- экспорт `GDMA_init_module()` для позднего запуска;
- исправления перехода в userspace.

`configs/rtd288o-tested.config` — полный проверенный config. После `olddefconfig`
строятся `Image`, `zImage`, `rtk_venusfb.ko` и `gdma-trigger.ko`.

## Два rootfs

Компактный initramfs содержит Alpine minirootfs, freestanding PID 1 и shell-loader.
PID 1 не зависит от libc: он сразу настраивает Ethernet, опрашивает `/dev/sys-misc`
командой MICOM `0xc0` примерно 25 раз/с и затем запускает shell-loader.

Loader скачивает полный rootfs по `ROOTFS_URL` в tmpfs 256 MiB, распаковывает его
в `/newroot`, удаляет архив и выполняет `chroot /newroot /init-alpine`.

Полный rootfs собирается через ARM binfmt/QEMU. Скрипт устанавливает Xorg fbdev,
Openbox, xterm и OpenSSH, компилирует musl `LD_PRELOAD` shim и добавляет только
публичный SSH-ключ. Host key создаётся при каждой RAM-загрузке.

## Выходные файлы

```text
dist/alpine-full-armv7.tar.gz
dist/gdma-trigger.ko
dist/rtk_venusfb.ko
dist/boot/uImage-rtd288o
dist/boot/rtd288o.dtb
dist/boot/alpine-loader.cpio.gz
dist/boot/kexec-arm
dist/boot/stage-manifest.json
```

Manifest содержит SHA-256, карту RAM, ожидаемое stock-ядро и command line. Его
проверяет телевизор до staging.
