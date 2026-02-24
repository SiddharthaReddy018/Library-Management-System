#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>

#define PORT 5000
#define BUFFER_SIZE 2046

void *handle_client(void *client_socket);
void send_message(int client_socket, const char *message);

pthread_mutex_t file_lock;

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;
    
    pthread_mutex_init(&file_lock, NULL);
    
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options to reuse address and port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Binding to the specified port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Server is listening on port %d\n", PORT);
    
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept failed");
            continue;
        }
        
        printf("New connection established.\n");
        
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, (void *)&new_socket);
        pthread_detach(thread_id);
    }
    
    pthread_mutex_destroy(&file_lock);
    close(server_fd);
    return 0;
}

void *handle_client(void *client_socket) {
    int sock = *(int *)client_socket;
    send_message(sock, "Connected to Library Management Server.\nLogout\n");
    close(sock);
    return NULL;
}

void send_message(int client_socket, const char *message) {
    send(client_socket, message, strlen(message), 0);
}
