#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

volatile sig_atomic_t keep_running = 1;

void handle_signal(int signo) {
    if (signo == SIGHUP) {
        printf("Received SIGHUP signal\n");
    }
    keep_running = 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int port = atoi(argv[1]);

    // Создание сокета
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return 1;
    }

    // Инициализация структуры адресов сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Привязка сокета к указанному порту
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_sock);
        return 1;
    }

    // Прослушивание входящего соединения
    if (listen(server_sock, 5) == -1) {
        perror("Listen failed");
        close(server_sock);
        return 1;
    }

    // Регистрация обработчика сигнала SIGHUP
    signal(SIGHUP, handle_signal);

    printf("Server is running on port %d\n", port);

    while (keep_running) {
        // Прием нового клиентского соединения
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
            perror("Accept failed");
            continue;
        }

        // Регистрация нового соединения
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("Accepted connection from %s:%d\n", client_ip, ntohs(client_addr.sin_port));

        // Считывание и обработка данных от клиента
        char buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = recv(client_sock, buffer, sizeof(buffer), 0)) > 0) {
            printf("Received %zd bytes from the client\n", bytes_read);
            //printf("Test bytes catched");
        }

        if (bytes_read == -1) {
            perror("Recv failed");
        }

        // Закрыть серверный сокет
        close(client_sock);
    }

    // Закрыть серверный сокет
    close(server_sock);
    printf("Server is shutting down\n");
    return 0;
}
