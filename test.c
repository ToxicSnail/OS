#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>
#include <errno.h>
#include <set>

// Глобальная переменная для управления циклом
volatile sig_atomic_t keep_running = 1;

// Обработчик сигнала SIGHUP
void handle_signal(int signo) {
    if (signo == SIGHUP) {
        // Действия при получении сигнала SIGHUP
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

    // Создание сокса сервера и настройка его параметров
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return 1;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_sock);
        return 1;
    }

    if (listen(server_sock, 5) == -1) {
        perror("Listen failed");
        close(server_sock);
        return 1;
    }

    signal(SIGHUP, handle_signal);

    printf("Server is running on port %d\n", port);

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(server_sock, &fds);

    std::set<int> clients;
    clients.insert(server_sock);
    int maxFd = server_sock;

    sigset_t origSigMask;
    sigemptyset(&origSigMask);
    sigaddset(&origSigMask, SIGHUP);

    // Заблокировать сигнал SIGHUP
    if (sigprocmask(SIG_BLOCK, &origSigMask, NULL) == -1) {
        perror("sigprocmask error");
        return 1;
    }

    while (keep_running) {
        fd_set temp_fds = fds;

        if (pselect(maxFd + 1, &temp_fds, NULL, NULL, NULL, &origSigMask) == -1) {
            if (errno == EINTR) {
                // Действия по обработке сигнала
                continue;
            }
            perror("pselect error");
            break;
        }

        for (int fd : clients) {
            if (FD_ISSET(fd, &temp_fds)) {
                if (fd == server_sock) {
                    // Обработка входящих соединений
                    if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
                        perror("Accept failed");
                    } else {
                        char client_ip[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
                        printf("Accepted connection from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
                        
                        clients.insert(client_sock);
                        FD_SET(client_sock, &fds);
                        
                        if (client_sock > maxFd) {
                            maxFd = client_sock;
                        }
                    }
                } else {
                    // Обработка активности на клиентском соксе
                    char buffer[1024];
                    ssize_t bytes_read = recv(fd, buffer, sizeof(buffer), 0);

                    if (bytes_read == -1) {
                        perror("Recv failed");
                    } else if (bytes_read == 0) {
                        printf("Connection closed by client\n");
                        close(fd);
                        FD_CLR(fd, &fds);
                        clients.erase(fd);
                    } else {
                        printf("Received %zd bytes from client\n", bytes_read);
                        // Обработка данных, если необходимо
                    }
                }
            }
        }
    }

    for (int fd : clients) {
        if (fd != server_sock) {
            close(fd);
        }
    }

    close(server_sock);
    printf("Server is shutting down\n");
    return 0;
}
