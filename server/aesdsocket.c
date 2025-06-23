#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>

#define LOG_SYS(fmt, ...) fprintf(stdout, "[SYS]: " fmt "\n", ##__VA_ARGS__)
#define LOG_ERR(fmt, ...) fprintf(stderr, "[ERROR]: " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) fprintf(stdout, "[DEBUG]: " fmt "\n", ##__VA_ARGS__)
#define MY_PORT 9000
#define BACKLOG 10
#define AESD_SOCKET_FILE "/var/tmp/aesdsocketdata.txt"
#define BUFFER_SIZE 1024

volatile sig_atomic_t exit_requested = 0;

void handle_signal(int signo) {
    //LOG_SYS("Received signal %d, shutting down gracefully...", signum);
    LOG_SYS("Caught signal, exiting");
    exit_requested = 1;
}

void setup_signal_handlers() {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

void setup_socket(int *sockfd, struct sockaddr_in *server_addr, char *server_ip) {
    // Create a socket using the provided argument as the protocol
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sockfd < 0) {
        LOG_ERR("Failed to create socket: %s", strerror(errno));
        exit(EXIT_FAILURE); // Exit if socket creation fails
    }
    LOG_SYS("Socket created successfully with descriptor: %d", *sockfd);
    int opt = 1;
    if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_ERR("setsockopt failed: %s", strerror(errno));
        close(*sockfd);
        exit(EXIT_FAILURE); // Exit if setsockopt fails
    }
    LOG_SYS("Socket options set successfully");
    // Bind the socket to a specific address and port
    if (bind(*sockfd, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        close(*sockfd); // Close the socket if binding fails
        LOG_ERR("Failed to bind socket: %s", strerror(errno));
        LOG_DEBUG("Server IP: %s, Port: %d", server_ip, ntohs(server_addr->sin_port));
        LOG_DEBUG("Socket descriptor: %d", *sockfd);
        LOG_DEBUG("Error number: %d", errno);
        exit(EXIT_FAILURE); // Exit if binding fails
    }
    LOG_SYS("Socket bound to %s:%d", inet_ntop(AF_INET, &server_addr->sin_addr, server_ip, INET_ADDRSTRLEN), ntohs(server_addr->sin_port));
    //Listen for incoming connections
    if (listen(*sockfd, BACKLOG) < 0) {
        LOG_ERR("Failed to listen on socket: %s", strerror(errno));
        close(*sockfd); // Close the socket if listening fails
        return -1; // Return an error code if listening fails
    }
    LOG_SYS("Listening on %s:%d", server_ip, ntohs(server_addr->sin_port));
    LOG_SYS("Waiting for incoming connections...");
}

void client_handler(int *sockfd, struct sockaddr_in *client_addr, char *client_ip) {
        socklen_t client_addr_len = sizeof(*client_addr); // Initialize the size of the client address structure 
    
    while (!exit_requested) {
        //inet_pton(AF_INET, client_ip, &client_addr.sin_addr); // Convert the IP address to a string
        // Accept an incoming connection
        int client_sock = accept(*sockfd, (struct sockaddr *)client_addr, &client_addr_len);
        if (client_sock < 0) {
            if (errno == EINTR) {
                // If interrupted by a signal, continue to accept new connections
                continue;
            }
            LOG_ERR("Failed to accept connection: %s", strerror(errno));
            close(*sockfd); // Close the socket if accepting connection fails
            return -1; // Return an error code if accepting connection fails
        }

        // Log the successful connection
        LOG_SYS("Accepted connection from %s:%d",
                inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof(client_ip)),
                ntohs(client_addr->sin_port));
        printf("Connection accepted from client\n");
        
        char *buffer = malloc(BUFFER_SIZE); // Allocate memory for the buffer
        if (!buffer) {
            LOG_ERR("Failed to allocate memory for buffer: %s", strerror(errno));
            close(client_sock); // Close the client socket if memory allocation fails
            continue; // Continue to the next iteration to accept new connections
        }
        size_t total_len = 0;
        ssize_t bytes_received;
        bool end_of_packet = false;

        FILE *file = fopen(AESD_SOCKET_FILE, "a");
        if (!file) {
            LOG_ERR("Failed to open file: %s", strerror(errno));
            close(client_sock); // Close the client socket if file opening fails
            free(buffer); // Free the allocated memory for buffer
            continue; // Continue to the next iteration to accept new connections
        }
        
        while (!end_of_packet && (bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0) {
            for (ssize_t i = 0; i < bytes_received; ++i) {
                if (buffer[i] == '\n') {
                    end_of_packet = true;
                    break;
                }
            } 
            // Check for newline in the received chunk
            // Write received chunk to file
            if (fwrite(buffer, 1, bytes_received, file) < bytes_received) {
                LOG_ERR("Failed to write to file: %s", strerror(errno));
                fclose(file);
                close(client_sock);
                free(buffer);
                continue;
            }
        }
        fclose(file); // Close the file after writing
        free(buffer); // Free the allocated memory for buffer
        LOG_SYS("Data written to %s", AESD_SOCKET_FILE);
        // Open the file for reading
        file = fopen(AESD_SOCKET_FILE, "r");
        if (!file) {
            LOG_ERR("Failed to open file for reading: %s", strerror(errno));
            close(client_sock); // Close the client socket if file opening fails
            continue; // Continue to the next iteration to accept new connections
        }
        // Read the file content and send it back to the client
        char *send_buffer = malloc(sizeof(char) * BUFFER_SIZE);
        if (!send_buffer) {
            LOG_ERR("Failed to allocate memory for send buffer: %s", strerror(errno));
            fclose(file); // Close the file if memory allocation fails
            close(client_sock); // Close the client socket
            free(send_buffer); // Free the allocated memory for buffer
            continue; // Continue to the next iteration to accept new connections
        }
        size_t byte_read;
        while ((byte_read = fread(send_buffer, 1, BUFFER_SIZE - 1, file)) > 0) {
            
            ssize_t bytes_sent = send(client_sock, send_buffer, byte_read, 0);
            if (bytes_sent < 0) {
                LOG_ERR("Failed to send data to client: %s", strerror(errno));
                fclose(file); // Close the file if sending fails
                close(client_sock); // Close the client socket
                free(send_buffer); // Free the allocated memory for send buffer
                continue; // Continue to the next iteration to accept new connections
            }
            LOG_SYS("Sent %zd bytes back to client", bytes_sent);
        }
        fclose(file); // Close the file after reading
        free(send_buffer); // Free the allocated memory for send buffer
        close(client_sock); // Close the client socket after handling
        LOG_SYS("Closed connection from %s:%d",
                inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof(client_ip)),
                ntohs(client_addr->sin_port));
    }
}

int main(int argc, char *argv[])
{
    struct sockaddr_in server_addr, client_addr;
    char server_ip[INET_ADDRSTRLEN]; // Buffer to hold the server IP address
    char client_ip[INET_ADDRSTRLEN];
    bool run_as_daemon = false;

    int opt;
    while ((opt = getopt(argc, argv, "d")) != -1) {
        switch (opt) {
            case 'd':
                run_as_daemon = true;
                break;
            default:
                fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    setup_signal_handlers(); // Set up signal handlers for graceful shutdown
    
    // Initialize the addrinfo structure
    server_addr.sin_family = AF_INET; // Set the address family to IPv4
    server_addr.sin_port = htons(9000); // Set the port number (example: 12345)
    server_addr.sin_addr.s_addr = INADDR_ANY; // Bind to all available interfaces
    int sockfd;
    

    //inet_pton(AF_INET, &server_addr.sin_addr, server_ip); // Convert the IP address to a string
    // Set up the socket
    setup_socket(&sockfd, &server_addr, server_ip);

    if (run_as_daemon) {
        // Fork the process to run as a daemon
        pid_t pid = fork();
        if (pid < 0) {
            LOG_ERR("Failed to fork process: %s", strerror(errno));
            close(sockfd);
            exit(EXIT_FAILURE); // Exit if fork fails
        }
        if (pid > 0) {
            // Parent process exits, leaving the child running in the background
            exit(EXIT_SUCCESS);
        }
        // Child process continues
        if (setsid() < 0) {
            LOG_ERR("Failed to create new session: %s", strerror(errno));
            close(sockfd);
            exit(EXIT_FAILURE); // Exit if setsid fails
        }

        close(STDIN_FILENO); // Close standard input
        close(STDOUT_FILENO); // Close standard output
        close(STDERR_FILENO); // Close standard error
        // Redirect standard input, output, and error to /dev/null
        open("/dev/null", O_RDONLY); // Redirect stdin to /dev/null
        open("/dev/null", O_WRONLY); // Redirect stdout to /dev/null
        open("/dev/null", O_WRONLY); // Redirect stderr to /dev/null
        LOG_SYS("Running as daemon...");
    }
    // Start listening for incoming connections
    client_handler(&sockfd, &client_addr, client_ip); // Handle client connections
    
    remove(AESD_SOCKET_FILE); // Remove the file if it exists
    close(sockfd);
    return 0; // Return success code
}