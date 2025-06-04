Консольная утилита для отправки команд на удалённый myRPC-сервер.

## Сборка

```bash
cd myRPC-client
make all
```

В результате появится исполняемый файл `myRPC-client`.

## Использование

```bash
./myRPC-client -s|--stream | -d|--dgram \
               -h <IP_сервера> \
               -p <порт> \
               -c|--command "bash команда"
```

Примеры:
```bash
./myRPC-client -s -h 192.168.0.10 -p 1234 -c "ls -la /tmp"
./myRPC-client --dgram --host 10.0.0.5 --port 4321 --command "df -h"
```