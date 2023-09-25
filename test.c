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
    // Этот обработчик для других сигналов, поэтому можно просто выйти
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

    // Create socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return 1;
    }

    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind socket to the specified port
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_sock);
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_sock, 5) == -1) {
        perror("Listen failed");
        close(server_sock);
        return 1;
    }

    printf("Server is running on port %d\n", port);

    // Use pselect for signal handling and incoming connections
    fd_set read_fds;
    struct timeval timeout;

    while (keep_running) {
        FD_ZERO(&read_fds);
        FD_SET(server_sock, &read_fds);

        timeout.tv_sec = 0;
        timeout.tv_usec = 500000; // 0.5 seconds

        int activity = pselect(server_sock + 1, &read_fds, NULL, NULL, &timeout, NULL);

        if (activity < 0 && errno != EINTR) {
            perror("pselect");
        }

        if (activity == 0) {
            // No activity, continue waiting
            continue;
        }

        if (FD_ISSET(server_sock, &read_fds)) {
            // Accept a new client connection
            if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
                perror("Accept failed");
                continue;
            }

            // Log the new connection
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
            printf("Accepted connection from %s:%d\n", client_ip, ntohs(client_addr.sin_port);

            // Read and process data from the client
            char buffer[1024];
            ssize_t bytes_read;
            while ((bytes_read = recv(client_sock, buffer, sizeof(buffer), 0)) > 0) {
                printf("Received %zd bytes from the client\n", bytes_read);
                // You can process the data here as needed
            }

            if (bytes_read == -1) {
                perror("Recv failed");
            }

            // Close the client socket
            close(client_sock);
        }
    }

    // Close the server socket
    close(server_sock);
    printf("Server is shutting down\n");
    return 0;
}
