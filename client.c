#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5000
#define BUFFER_SIZE 2046

void send_message(int sock, const char *message);
void receive_message(int sock, char *buffer);

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    printf("Connected to server.\n");

    while (1) {
        receive_message(sock, buffer);
        printf("Server message: %s\n", buffer);

        if (strcmp(buffer, "Logout\n") == 0) {
            printf("Logged out successfully.\n");
            break;
        }

        printf("Enter message: ");
        if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
            send_message(sock, buffer);
        }
    }

    close(sock);
    return 0;
}

void send_message(int sock, const char *message) {
    send(sock, message, strlen(message), 0);
}

void receive_message(int sock, char *buffer) {
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_read = read(sock, buffer, BUFFER_SIZE - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
    } else if (bytes_read == 0) {
        printf("Server closed the connection.\n");
        exit(0);
    } else {
        perror("Error reading from server");
        exit(1);
    }
}
