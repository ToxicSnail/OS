#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h> // Add this include for struct sockaddr_in
#include <time.h>       // Add this include for struct timespec

#define PORT 8080
#define MAX_CONNECTIONS 5

volatile sig_atomic_t is_running = 1;

void signal_handler(int signo) {
    if (signo == SIGHUP) {
        printf("Received SIGHUP signal\n");
        // Additional handling for SIGHUP (as needed)
    }
}

int main() {
    // Set up a signal handler for SIGHUP
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGHUP, &sa, NULL);

    // Create a socket
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

    // Bind the socket to the address and port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening on the port
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
        timeout.tv_sec = 1;  // Wait for 1 second
        timeout.tv_nsec = 0;

        int result = pselect(max_fd + 1, &temp_fds, NULL, NULL, &timeout, NULL);
        if (result == -1) {
            perror("pselect");
            continue;
        } else if (result == 0) {
            // Timeout occurred, you can perform some actions here if needed
        }

        for (int fd = 0; fd <= max_fd; fd++) {
            if (FD_ISSET(fd, &temp_fds)) {
                // Handle incoming connections and data from clients
                // ...
            }
        }
    }

    // Close the server socket and free resources
    close(server_socket);

    return 0;
}
