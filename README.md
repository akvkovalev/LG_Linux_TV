# Native Alpine Linux on LG 43UN71006LB

Воспроизводимый экспериментальный запуск Alpine Linux 3.22/ARMv7 на телевизоре
LG 43UN71006LB через `kexec`. Ядро, initramfs и полный rootfs работают из RAM;
загрузчик и разделы eMMC не изменяются. После перезагрузки телевизор возвращается
в webOS.

Проверенная конфигурация:

- LG 43UN71006LB, webOS 5.6.1, ПО 04.63.25;
- Realtek `rtd288o`, четыре ARMv7 ядра;
- исходники LG `kernel-1.0.0-612.21`, Linux 4.4.84;
- Alpine 3.22.6 `armv7`;
- Ethernet, статический адрес;
- GDMA + `venusfb`, framebuffer 1920×1080×16;
- Xorg fbdev, Openbox и xterm;
- SSH по ключу на порту 2222.

## Что уже работает

- отдельное ядро и настоящий PID 1, без chroot поверх ядра webOS;
- MICOM keepalive, благодаря которому система работает дольше штатного тайм-аута;
- Ethernet, SSH, Alpine `apk`, Python, NFS/CIFS;
- автоматическая поздняя инициализация GDMA и `venusfb`;
- автоматический старт Xorg/Openbox;
- безопасная предварительная проверка `kexec -l` → `kexec -u`.

```text
signed LG boot chain → stock webOS kernel → kexec
    → patched LG kernel
    → compact initramfs + freestanding PID 1
        ├─ MICOM poll ~25 Hz
        └─ download full Alpine rootfs over HTTP
            → chroot → GDMA → venusfb → Xorg/Openbox
```

## Быстрый старт

Нужен Linux-сервер, rooted webOS с root SSH, Ethernet и исходный архив ядра LG.
Собранные образы и проприетарные исходники LG в репозиторий не входят.

```sh
git clone https://github.com/akvkovalev/LG_Linux_TV.git
cd LG_Linux_TV
cp config/example.env .env
$EDITOR .env

./scripts/apply-kernel-patch.sh
./scripts/build-kernel.sh
./scripts/build-gdma-trigger.sh
./scripts/build-gdma-show.sh
./scripts/build-stage1.sh
./scripts/build-loader-initramfs.sh
./scripts/build-kexec.sh
sudo -E ./scripts/build-full-rootfs.sh
./scripts/build-boot-bundle.sh
```

В одном терминале запустите HTTP-сервер полного rootfs:

```sh
./scripts/serve-rootfs.sh
```

В другом — netconsole и безопасную проверку загрузочного набора:

```sh
python3 tools/collect_netconsole.py
./scripts/deploy.sh
```

`deploy.sh` только загружает новый образ в kexec и сразу выгружает его. Если
проверка завершилась `success: true`, RAM-запуск выполняется отдельно:

```sh
./scripts/boot.sh
```

После появления Alpine:

```sh
ssh -p 2222 -i ~/.ssh/id_devman root@192.168.89.76
```

Подробности: [сборка](docs/build.md), [запуск](docs/run.md),
[устройство системы](docs/architecture.md), [восстановление](docs/recovery.md)
и [известные ограничения](docs/troubleshooting.md).

## Важные границы

DTB, карта RAM, параметры панели и патч ядра проверены только на указанной модели
и версии ПО. Скрипт на телевизоре сверяет модель SoC, версию stock-ядра, карту RAM
и SHA-256 каждого файла до загрузки. При несовпадении он прекращает работу.

Запуск остаётся экспериментальным. Во время `kexec -e` экран может погаснуть, а
при ошибке ядра телевизор может перестать отвечать до отключения питания. Пока
система не пишет в eMMC, обычное отключение питания возвращает штатную webOS.

Полезный предшествующий проект: [lg-webos-kexec](https://github.com/ggfuchsi-oss/lg-webos-kexec).
