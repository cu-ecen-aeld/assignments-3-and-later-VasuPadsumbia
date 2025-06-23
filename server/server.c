int main(int argc, char *argv[])
{
    // Check if the correct number of arguments is provided
    if (argc != 2) {
        return 1; // Return an error code if the argument count is incorrect
    }
    struct sockaddr_in server_addr, client_addr;
    char *Data;
    char ip_address[INET_ADDRSTRLEN];
    // Initialize the addrinfo structure
    server_addr.sin_family = AF_INET; // Set the address family to IPv4
    server_addr.sin_port = htons(9000); // Set the port number (example: 12345)
    server_addr.sin_addr.s_addr = INADDR_ANY; // Bind to all available interfaces
    // Check if the provided argument is a valid protocol number
    if (atoi(argv[1]) <= 0) {
        return 1; // Return an error code if the protocol number is invalid
    }
    inet_pton(AF_INET, &server_addr.sin_addr, ip_address); // Convert the IP address to a string
    printf("Server will bind to IP: %s, Port: %d\n", ip_address, ntohs(server_addr.sin_port));
    // Create a socket using the provided argument as the protocol
    int sockfd = socket(AF_INET, SOCK_STREAM, atoi(argv[1]));
    if (sockfd < 0) {
        return -1; // Return an error code if socket creation fails
    }
    // Bind the socket to a specific address and port
    bind (sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    // Connect the socket to the server address
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd); // Close the socket if connection fails
        return -1; // Return an error code if connection fails
    }
    LOG_SYS("Connected to %s:%d", ip_address, ntohs(server_addr.sin_port));

    //Listen for incoming connections
    if (listen(sockfd, 5) < 0) {
        close(sockfd); // Close the socket if listening fails
        return -1; // Return an error code if listening fails
    }
    LOG_SYS("Listening on %s:%d", ip_address, ntohs(server_addr.sin_port));
    
    // Accept an incoming connection
    int client_sock = accept(sockfd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr);
    if (client_sock < 0) {
        close(sockfd); // Close the socket if accepting connection fails
        return -1; // Return an error code if accepting connection fails
    }
    // Log the successful connection
    LOG_SYS("Accepted connection from %s:%d",
            inet_ntop(AF_INET, &client_addr.sin_addr, ip_address, sizeof(ip_address)),
            ntohs(client_addr.sin_port));
    printf("Connection accepted from client\n");
    
    char *buffer;
    // Recieving data from the client
    if (recv(client_sock, *buffer, sizeof(ip_address) - 1, 0) < 0) {
        close(client_sock); // Close the client socket if receiving fails
        close(sockfd); // Close the server socket
        return -1; // Return an error code if receiving fails
    }
    // Log the received data
    LOG_SYS("Received data from client: %s", buffer);

    FILE *file = fopen("/var/tmp/aesdsocketdata.txt", "w");
    if (!file) {
        close(client_sock); // Close the client socket if file opening fails
        close(sockfd); // Close the server socket
        return -1; // Return an error code if file opening fails
    }
    // Write the received data to the file
    if (fprintf(file, "%s\n", buffer) < 0) {
        fclose(file); // Close the file if writing fails
        close(client_sock); // Close the client socket
        close(sockfd); // Close the server socket
        return -1; // Return an error code if writing fails     
    }
    fclose(file); // Close the file after writing
    LOG_SYS("Data written to /var/tmp/aesdsocketdata.txt");

    close(client_sock); // Close the client socket after handling
    // Close the socket before exiting
    close(sockfd);
    return 0; // Return success code
}