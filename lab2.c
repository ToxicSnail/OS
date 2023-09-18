#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CONNECTIONS 5

volatile sig_atomic_t is_running = 1;

void signal_handler(int signo) {
    if (signo == SIGHUP) {
        printf("Received SIGHUP signal\n");
        // Дополнительная обработка SIGHUP (по вашему выбору)
    }
}

int main() {
    // Установка обработчика сигнала SIGHUP
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction)); // Исправленная строка
    sa.sa_handler = signal_handler;
    sigaction(SIGHUP, &sa, NULL);

    // Создание сокета
    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    // Привязка соксета к адресу и порту
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Начало прослушивания порта
    if (listen(server_socket, MAX_CONNECTIONS) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    fd_set read_fds, temp_fds;
    FD_ZERO(&read_fds);
    FD_SET(server_socket, &read_fds);
    int max_fd = server_socket;

    while (is_running) {
        temp_fds = read_fds;

        struct timespec timeout;
        timeout.tv_sec = 1;  // Ждем 1 секунду
        timeout.tv_nsec = 0;

        int result = pselect(max_fd + 1, &temp_fds, NULL, NULL, &timeout, NULL);
        if (result == -1) {
            perror("pselect");
            continue;
        } else if (result == 0) {
            // Время ожидания истекло, можно выполнить какие-либо действия
        }

        for (int fd = 0; fd <= max_fd; fd++) {
            if (FD_ISSET(fd, &temp_fds)) {
                if (fd == server_socket) {
                    // Новое входящее соединение
                    int client_socket;
                    if ((client_socket = accept(server_socket, NULL, NULL)) == -1) {
                        perror("Accept failed");
                        continue;
                    }

                    printf("New connection accepted\n");

                    // Закрываем все остальные соединения
                    for (int i = 0; i <= max_fd; i++) {
                        if (FD_ISSET(i, &read_fds) && i != server_socket && i != client_socket) {
                            close(i);
                            FD_CLR(i, &read_fds);
                        }
                    }

                    FD_SET(client_socket, &read_fds);
                    if (client_socket > max_fd) {
                        max_fd = client_socket;
                    }
                } else {
                    // Данные в существующем соединении
                    char buffer[1024];
                    ssize_t bytes_read = recv(fd, buffer, sizeof(buffer), 0);
                    if (bytes_read <= 0) {
                        // Соединение закрыто или произошла ошибка
                        printf("Connection closed\n");
                        close(fd);
                        FD_CLR(fd, &read_fds);
                    } else {
                        buffer[bytes_read] = '\0';
                        printf("Received data from client: %s\n", buffer);
                    }
                }
            }
        }
    }

    // Закрытие серверного соксета и освобождение ресурсов
    close(server_socket);

    return 0;
}
