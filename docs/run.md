# Запуск

Телевизор и netconsole-приёмник должны быть в одном Ethernet-сегменте. HTTP-сервер
может находиться за маршрутизатором. Wi-Fi для первого этапа не используется.

## 1. HTTP и netconsole

```sh
./scripts/serve-rootfs.sh
python3 tools/collect_netconsole.py --output boot.jsonl
```

Для UDP 6666 может потребоваться правило firewall.

## 2. Safe staging

```sh
./scripts/deploy.sh
```

Команда копирует bundle в `/tmp/lg-linux-tv`, проверяет хеши, stock kernel,
`rtd288o` и карту RAM. Затем выполняет kexec load/unload. Экран и webOS продолжают
работать.

## 3. RAM boot

Оставьте телевизор включённым и выполните:

```sh
./scripts/boot.sh
```

Перед переходом скрипт сохраняет и временно отключает SB2/ARM/DCMT мониторы,
которые иначе могут остановить новый kernel. Если `kexec` возвращает ошибку,
monitor state восстанавливается.

Ожидаемая последовательность netconsole:

```text
LGTV LOADER NETWORK READY
LGTV FULL ALPINE EXTRACTED
```

После загрузки:

```sh
ssh -p 2222 -i "$SSH_KEY" root@"$TV_HOST"
uname -a
cat /proc/fb
cat /sys/class/graphics/fb0/virtual_size
ps | grep -E 'Xorg|openbox|xterm'
```

Для возврата выполните `sync; reboot -f`. Если SSH недоступен, отключите питание
на 15 секунд и включите телевизор.
