#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

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

    fd_set read_fds;
    int max_fd;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_sock, &read_fds);
        max_fd = server_sock;

        // Accept new client connections
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
            perror("Accept failed");
            continue;
        }

        FD_SET(client_sock, &read_fds);
        if (client_sock > max_fd) {
            max_fd = client_sock;
        }

        // Log the new connection
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("Accepted connection from %s:%d\n", client_ip, ntohs(client_addr.sin_port));

        // Read and process data from the client
        char buffer[1024];
        ssize_t bytes_read;

        while (1) {
            fd_set tmp_fds = read_fds;
            if (pselect(max_fd + 1, &tmp_fds, NULL, NULL, NULL, NULL) == -1) {
                perror("pselect failed");
                break;
            }

            for (int fd = 0; fd <= max_fd; fd++) {
                if (FD_ISSET(fd, &tmp_fds)) {
                    if (fd == server_sock) {
                        // New client connection
                        continue;
                    } else {
                        // Data from client
                        bytes_read = recv(fd, buffer, sizeof(buffer), 0);
                        if (bytes_read <= 0) {
                            // Connection closed or error
                            if (bytes_read == 0) {
                                printf("Client %d disconnected\n", fd);
                            } else {
                                perror("Recv failed");
                            }
                            close(fd);
                            FD_CLR(fd, &read_fds);
                        } else {
                            printf("Received %zd bytes from client %d\n", bytes_read, fd);
                            // You can process the data here as needed
                        }
                    }
                }
            }
        }
    }

    // Close the server socket
    close(server_sock);
    printf("Server is shutting down\n");
    return 0;
}
