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
void admin_menu(int client_socket);
void user_menu(int client_socket, const char *username);
void send_message(int client_socket, const char *message);
int authenticate(const char *username, const char *password, char *role);

pthread_mutex_t file_lock;
bool is_logged_in[50] = {false};

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;
    
    pthread_mutex_init(&file_lock, NULL);
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
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

int authenticate(const char *username, const char *password, char *role) {
    FILE *file;
    char line[100];
    char stored_user[50], stored_pass[50], stored_role[10];
    int authenticated = 0;
    int user_index = -1;

    pthread_mutex_lock(&file_lock);
    file = fopen("users.txt", "r");

    if (!file) {
        perror("Could not open users.txt");
        pthread_mutex_unlock(&file_lock);
        return 0;
    }

    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%s %s %s", stored_user, stored_pass, stored_role);
        if (strcmp(stored_user, username) == 0 && strcmp(stored_pass, password) == 0) {
            authenticated = 1;
            strcpy(role, stored_role);
            user_index = atoi(stored_user);
            break;
        }
    }

    fclose(file);
    pthread_mutex_unlock(&file_lock);

    if (authenticated && !is_logged_in[user_index]) {
        is_logged_in[user_index] = true;
    } else if (authenticated && is_logged_in[user_index]) {
        printf("User already logged in\n");
        authenticated = 0;
    }

    return authenticated;
}

void admin_menu(int client_socket) {
    send_message(client_socket, "\nAdmin Menu:\n10. Logout\nChoice: ");
}

void user_menu(int client_socket, const char *username) {
    send_message(client_socket, "\nUser Menu:\n4. Logout\nChoice: ");
}

void *handle_client(void *client_socket) {
    int sock = *(int *)client_socket;
    char username[50], password[50], role[10];

    send_message(sock, "Enter username: ");
    if (read(sock, username, 50) <= 0) {
        perror("Read error");
        close(sock);
        return NULL;
    }
    username[strcspn(username, "\n")] = '\0';

    send_message(sock, "Enter password: ");
    if (read(sock, password, 50) <= 0) {
        perror("Read error");
        close(sock);
        return NULL;
    }
    password[strcspn(password, "\n")] = '\0';

    if (authenticate(username, password, role)) {
        if (strcmp(role, "admin") == 0) {
            admin_menu(sock);
        } else {
            user_menu(sock, username);
        }
    } else {
        send_message(sock, "Authentication failed.\n\n");
    }

    send_message(sock, "Logout\n");
    is_logged_in[atoi(username)] = false;
    close(sock);
    return NULL;
}

void send_message(int client_socket, const char *message) {
    send(client_socket, message, strlen(message), 0);
}
