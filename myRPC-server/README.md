Служба-демон для удалённого выполнения команд по протоколу myRPC.

## Зависимости

- GCC (gcc)
- libconfig (`libconfig-dev`)
- libmysyslog (собирается в подпроекте `libmysyslog`)
- pkg-config

Установите зависимости:
```bash
sudo apt-get update
sudo apt-get install -y gcc make libconfig-dev pkg-config
```

## Сборка

```bash
cd myRPC-server
make all
```

- Соберётся бинарник `myRPC-server` в текущей папке.
- Он автоматически требует, чтобы в соседней папке `../libmysyslog` уже была собрана библиотека `libmysyslog.a` или `.so`. Если не собрана, запустите там `make all`.

## Настройка

1. Создайте каталог `/etc/myRPC`:
   ```bash
   sudo mkdir -p /etc/myRPC
   ```
2. Файл `/etc/myRPC/myRPC.conf`:
   ```conf
   port = 1234;
   socket_type = "stream";   # stream или dgram
   ```
3. Файл `/etc/myRPC/users.conf` — список разрешённых пользователей, по одному на строку:
   ```
   alice
   bob
   ```

## Запуск
```bash
sudo ./myRPC-server
```

## Пакет

Создать DEB-пакет:
```bash
make deb
```
Получится `MyRPC_server_1.0-0_all.deb`. Устанавливается командой:
```bash
sudo dpkg -i MyRPC_server_1.0-0_all.deb
```