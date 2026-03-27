#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5000 // Ensure this matches with server
#define BUFFER_SIZE 2046

void send_message(int sock, const char *message);
void receive_message(int sock, char *buffer);

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    int loggedIn = 0;
    char username[50];

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
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
            loggedIn = 0;
            break;
        }

        if (!loggedIn && (strstr(buffer, "Enter username") != NULL || strstr(buffer, "Enter password") != NULL) || strstr(buffer,"Enter role")!=NULL) {
            printf("Enter your message: ");
            fgets(buffer, BUFFER_SIZE, stdin);
            send_message(sock, buffer);

            if (strstr(buffer, "admin") != NULL) {
                strcpy(username, "admin");
                loggedIn = 1;
            } else {
                strcpy(username, buffer);
                loggedIn = 1;
            }
        } else if (loggedIn && strstr(buffer, "Choice:") != NULL) {
            printf("Enter your choice: ");
            fgets(buffer, BUFFER_SIZE, stdin);
            send_message(sock, buffer);
        } else if (loggedIn && (strstr(buffer, "ISBN") != NULL || strstr(buffer, "username") != NULL || strstr(buffer, "password") != NULL || strstr(buffer, "Title") != NULL || strstr(buffer, "Author") != NULL)) {
            printf("Enter your input: ");
            fgets(buffer, BUFFER_SIZE, stdin);
            send_message(sock, buffer);
        } else if (loggedIn && strcmp(username, "admin") != 0 && strstr(buffer, "User Menu:") != NULL) {
            printf("Enter your choice: ");
            fgets(buffer, BUFFER_SIZE, stdin);
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