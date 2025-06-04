# Библиотека libmysyslog

Лёгкая логирующая библиотека для проекта myRPC. Предназначена для вывода сообщений в текстовом или структурированном (JSON) формате.

## Поддерживаемые форматы

- **TEXT** — стандартная строка: время, уровень, сообщение.
- **JSON** — объект с полями `timestamp`, `severity`, `message`.

## Уровни логирования

- TRACE
- NOTE
- ALERT
- FAILURE
- FATAL

## Использование

```c
int push_log(const char* message, int level, int driver, int format, const char* filepath);
```

## Сборка

```bash
make
```
