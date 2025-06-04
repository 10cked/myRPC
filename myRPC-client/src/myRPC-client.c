#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 2048

void print_usage(const char *prog_name) {
    printf("Использование: %s -s|--stream или -d|--dgram -h <ip_addr> -p <port> -c|--command \"bash команда\"\n", prog_name);
}

int main(int argc, char *argv[]) {
    int use_stream = 1; // По умолчанию потоковый сокет
    char host[256] = "127.0.0.1";
    int port = 1234;
    char command[512] = {0};

    // Парсинг аргументов командной строки
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--stream") == 0) {
            use_stream = 1;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dgram") == 0) {
            use_stream = 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--host") == 0) {
            if (++i < argc)
                strncpy(host, argv[i], sizeof(host) - 1);
            else {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if (++i < argc)
                port = atoi(argv[i]);
            else {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--command") == 0) {
            if (++i < argc)
                strncpy(command, argv[i], sizeof(command) - 1);
            else {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        }
    }

    if (strlen(command) == 0) {
        fprintf(stderr, "Команда не задана.\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    char *username = getlogin();
    if (username == NULL) {
        perror("Ошибка получения имени пользователя");
        exit(EXIT_FAILURE);
    }

    // Формирование запроса в формате JSON
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "{\"login\":\"%s\",\"command\":\"%s\"}", username, command);

    // Создание сокета
    int sock;
    if (use_stream)
        sock = socket(AF_INET, SOCK_STREAM, 0);
    else
        sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        perror("Ошибка создания сокета");
        exit(EXIT_FAILURE);
    }

    // Настройка адреса сервера
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        perror("Ошибка адреса");
        exit(EXIT_FAILURE);
    }

    // Если используется потоковый сокет, устанавливаем соединение
    if (use_stream) {
        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("Ошибка подключения к серверу");
            exit(EXIT_FAILURE);
        }
    }

    // Отправка запроса серверу
    ssize_t sent_bytes = sendto(sock, request, strlen(request), 0,
                                (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (sent_bytes < 0) {
        perror("Ошибка отправки данных");
        exit(EXIT_FAILURE);
    }

    // Получение ответа от сервера
    char response[BUFFER_SIZE] = {0};
    ssize_t received_bytes = recvfrom(sock, response, sizeof(response) - 1, 0, NULL, NULL);
    if (received_bytes < 0) {
        perror("Ошибка получения ответа");
        exit(EXIT_FAILURE);
    }
    printf("Ответ сервера: %s\n", response);

    close(sock);
    return 0;
}
