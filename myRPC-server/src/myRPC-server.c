#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libconfig.h>       // Для разбора конфигурационных файлов
#include "libmysyslog.h"     // Заголовочный файл библиотеки логирования

#define CONFIG_FILE "/etc/myRPC/myRPC.conf"
#define USERS_FILE "/etc/myRPC/users.conf"
#define BUFFER_SIZE 2048

// Задаём значения по умолчанию для дополнительных параметров логирования
#define DEFAULT_DRIVER 0
#define DEFAULT_FORMAT 0
#define DEFAULT_PATH ""

// Функция проверки, разрешён ли пользователь
int is_user_allowed(const char *username) {
    FILE *file = fopen(USERS_FILE, "r");
    if (!file) {
        mysyslog("[myRPC-server] Ошибка открытия файла с пользователями", 1, DEFAULT_DRIVER, DEFAULT_FORMAT, DEFAULT_PATH);
        return 0;
    }
    char line[128];
    int allowed = 0;
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
        if (strcmp(line, username) == 0) {
            allowed = 1;
            break;
        }
    }
    fclose(file);
    return allowed;
}

// Функция выполнения команды с перенаправлением вывода во временные файлы
void execute_and_save(const char *command, char *tmp_stdout, char *tmp_stderr) {
    char exec_command[1024];
    snprintf(exec_command, sizeof(exec_command), "%s > %s 2> %s", command, tmp_stdout, tmp_stderr);
    system(exec_command);
}

int main() {
    // Логируем инициализацию сервера
    mysyslog("[myRPC-server] Инициализация сервера", 0, DEFAULT_DRIVER, DEFAULT_FORMAT, DEFAULT_PATH);

    // Чтение конфигурационного файла
    config_t cfg;
    config_init(&cfg);
    if (!config_read_file(&cfg, CONFIG_FILE)) {
        mysyslog("[myRPC-server] Ошибка чтения конфигурационного файла", 1, DEFAULT_DRIVER, DEFAULT_FORMAT, DEFAULT_PATH);
        exit(EXIT_FAILURE);
    }
    
    int port;
    if (!config_lookup_int(&cfg, "port", &port)) {
        mysyslog("[myRPC-server] Не найден параметр port в конфигурационном файле", 1, DEFAULT_DRIVER, DEFAULT_FORMAT, DEFAULT_PATH);
        exit(EXIT_FAILURE);
    }
    
    const char *socket_type_str;
    if (!config_lookup_string(&cfg, "socket_type", &socket_type_str)) {
        mysyslog("[myRPC-server] Не найден параметр socket_type в конфигурационном файле", 1, DEFAULT_DRIVER, DEFAULT_FORMAT, DEFAULT_PATH);
        exit(EXIT_FAILURE);
    }
    
    int use_stream = 1; // По умолчанию потоковый сокет
    if (strcmp(socket_type_str, "dgram") == 0) {
        use_stream = 0;
    }
    config_destroy(&cfg);

    // Создание сокета
    int sock;
    if (use_stream)
        sock = socket(AF_INET, SOCK_STREAM, 0);
    else
        sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        mysyslog("[myRPC-server] Ошибка создания сокета", 1, DEFAULT_DRIVER, DEFAULT_FORMAT, DEFAULT_PATH);
        exit(EXIT_FAILURE);
    }

    // Настройка адреса сервера
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        mysyslog("[myRPC-server] Ошибка привязки сокета", 1, DEFAULT_DRIVER, DEFAULT_FORMAT, DEFAULT_PATH);
        exit(EXIT_FAILURE);
    }

    if (use_stream && listen(sock, 5) < 0) {
        mysyslog("[myRPC-server] Ошибка прослушивания сокета", 1, DEFAULT_DRIVER, DEFAULT_FORMAT, DEFAULT_PATH);
        exit(EXIT_FAILURE);
    }

    mysyslog("[myRPC-server] Сервер запущен", 0, DEFAULT_DRIVER, DEFAULT_FORMAT, DEFAULT_PATH);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        char buffer[BUFFER_SIZE] = {0};

        int client_sock = sock;
        if (use_stream) {
            client_sock = accept(sock, (struct sockaddr*)&client_addr, &addr_len);
            if (client_sock < 0) {
                mysyslog("[myRPC-server] Ошибка при принятии соединения", 1, DEFAULT_DRIVER, DEFAULT_FORMAT, DEFAULT_PATH);
                continue;
            }
        }

        ssize_t received_bytes = recvfrom(client_sock, buffer, sizeof(buffer) - 1, 0,
                                          (struct sockaddr*)&client_addr, &addr_len);
        if (received_bytes < 0) {
            mysyslog("[myRPC-server] Ошибка получения данных", 1, DEFAULT_DRIVER, DEFAULT_FORMAT, DEFAULT_PATH);
            if (use_stream)
                close(client_sock);
            continue;
        }
        buffer[received_bytes] = '\0';
        mysyslog("[myRPC-server] Получен запрос", 0, DEFAULT_DRIVER, DEFAULT_FORMAT, DEFAULT_PATH);

        // Разбор JSON-формата (ожидается {"login":"user","command":"bash команда"})
        char login[128] = {0}, command[512] = {0};
        char *login_ptr = strstr(buffer, "\"login\"");
        char *command_ptr = strstr(buffer, "\"command\"");
        if (login_ptr && command_ptr) {
            sscanf(login_ptr, "\"login\":\"%[^\"]\"", login);
            sscanf(command_ptr, "\"command\":\"%[^\"]\"", command);
        } else {
            const char *error_response = "{\"code\":1, \"result\":\"Неверный формат запроса\"}";
            sendto(client_sock, error_response, strlen(error_response), 0,
                   (struct sockaddr*)&client_addr, addr_len);
            if (use_stream)
                close(client_sock);
            continue;
        }

        if (!is_user_allowed(login)) {
            const char *deny_response = "{\"code\":1, \"result\":\"Пользователь не разрешён\"}";
            sendto(client_sock, deny_response, strlen(deny_response), 0,
                   (struct sockaddr*)&client_addr, addr_len);
            if (use_stream)
                close(client_sock);
            continue;
        }

        char tmp_id[16];
        snprintf(tmp_id, sizeof(tmp_id), "%d", (int)getpid());
        char tmp_stdout[256], tmp_stderr[256];
        snprintf(tmp_stdout, sizeof(tmp_stdout), "/tmp/myRPC_%s.stdout", tmp_id);
        snprintf(tmp_stderr, sizeof(tmp_stderr), "/tmp/myRPC_%s.stderr", tmp_id);

        execute_and_save(command, tmp_stdout, tmp_stderr);

        char result[1024] = {0};
        FILE *f_stdout = fopen(tmp_stdout, "r");
        if (f_stdout) {
            fread(result, 1, sizeof(result) - 1, f_stdout);
            fclose(f_stdout);
        }
        FILE *f_stderr = fopen(tmp_stderr, "r");
        if (f_stderr) {
            char err_buf[1024] = {0};
            fread(err_buf, 1, sizeof(err_buf) - 1, f_stderr);
            fclose(f_stderr);
            if (strlen(err_buf) > 0) {
                char response[2048];
                snprintf(response, sizeof(response), "{\"code\":1, \"result\":\"%s\"}", err_buf);
                sendto(client_sock, response, strlen(response), 0,
                       (struct sockaddr*)&client_addr, addr_len);
                if (use_stream)
                    close(client_sock);
                continue;
            }
        }

        char response[2048];
        snprintf(response, sizeof(response), "{\"code\":0, \"result\":\"%s\"}", result);
        sendto(client_sock, response, strlen(response), 0,
               (struct sockaddr*)&client_addr, addr_len);

        if (use_stream)
            close(client_sock);
    }

    close(sock);
    return 0;
}
