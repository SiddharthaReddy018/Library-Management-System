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
void user_menu(int client_socket,const char *username);
void add_book(int client_socket);
void delete_book(int client_socket);
void search_book(int client_socket);
void view_all_books(int client_socket);
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

void user_menu(int client_socket, const char *username) {
    send_message(client_socket, "\nUser Menu:\n4. Logout\nChoice: ");
}

void add_book(int client_socket) {
    char title[50], author[50], isbn[4], buffer[BUFFER_SIZE], line[100];
    int found = 0;

    send_message(client_socket, "Title: ");
    read(client_socket, title, 50);
    title[strcspn(title, "\n")] = '\0';

    send_message(client_socket, "Author: ");
    read(client_socket, author, 50);
    author[strcspn(author, "\n")] = '\0';

    send_message(client_socket, "ISBN (3 digits): ");
    read(client_socket, isbn, 4);
    isbn[strcspn(isbn, "\n")] = '\0';

    pthread_mutex_lock(&file_lock);
    FILE *file = fopen("books.txt", "r");

    if (file) {
        while (fgets(line, sizeof(line), file)) {
            char record_isbn[4];
            sscanf(line, "%3[^,]", record_isbn);
            if (strcmp(record_isbn, isbn) == 0) {
                found = 1;
                break;
            }
        }
        fclose(file);
    }

    if (found) {
        pthread_mutex_unlock(&file_lock);
        snprintf(buffer, sizeof(buffer), "Book with ISBN '%s' already exists.\n\n", isbn);
        send_message(client_socket, buffer);
        return;
    }

    file = fopen("books.txt", "a");
    if (!file) {
        perror("Could not open books.txt");
        pthread_mutex_unlock(&file_lock);
        send_message(client_socket, "Failed to add book.\n\n");
        return;
    }

    fprintf(file, "%s,%s,%s,1\n", isbn, title, author);
    fclose(file);
    pthread_mutex_unlock(&file_lock);

    send_message(client_socket, "Book added successfully.\n\n");
}

void delete_book(int client_socket) {
    char isbn[4], buffer[BUFFER_SIZE], line[100];
    int found = 0;

    send_message(client_socket, "Enter ISBN of book to delete (3 digits): ");
    read(client_socket, isbn, 4);
    isbn[strcspn(isbn, "\n")] = '\0';

    pthread_mutex_lock(&file_lock);
    FILE *file = fopen("books.txt", "r");
    FILE *temp = fopen("temp.txt", "w");

    if (!file || !temp) {
        perror("File error");
        pthread_mutex_unlock(&file_lock);
        send_message(client_socket, "Failed to delete book.\n\n");
        return;
    }

    while (fgets(line, sizeof(line), file)) {
        char record_isbn[4];
        sscanf(line, "%3[^,]", record_isbn);
        if (strcmp(record_isbn, isbn) == 0) {
            found = 1;
        } else {
            fputs(line, temp);
        }
    }

    fclose(file);
    fclose(temp);

    if (found) {
        remove("books.txt");
        rename("temp.txt", "books.txt");
        send_message(client_socket, "Book deleted successfully.\n\n");
    } else {
        remove("temp.txt");
        send_message(client_socket, "Book not found.\n\n");
    }
    pthread_mutex_unlock(&file_lock);
}

void search_book(int client_socket) {
    char isbn[4], buffer[BUFFER_SIZE], line[100];
    int found = 0;

    send_message(client_socket, "Enter ISBN of book to search (3 digits): ");
    read(client_socket, isbn, 4);
    isbn[strcspn(isbn, "\n")] = '\0';

    pthread_mutex_lock(&file_lock);
    FILE *file = fopen("books.txt", "r");

    if (!file) {
        perror("File error");
        pthread_mutex_unlock(&file_lock);
        send_message(client_socket, "Failed to open books file.\n\n");
        return;
    }

    while (fgets(line, sizeof(line), file)) {
        char record_isbn[4], title[50], author[50];
        int available;
        sscanf(line, "%3[^,],%49[^,],%49[^,],%d", record_isbn, title, author, &available);
        if (strcmp(record_isbn, isbn) == 0) {
            snprintf(buffer, sizeof(buffer), "Found Book: ISBN: %s, Title: %s, Author: %s, Available: %s\n\n",
                     record_isbn, title, author, available ? "Yes" : "No");
            send_message(client_socket, buffer);
            found = 1;
            break;
        }
    }

    fclose(file);
    pthread_mutex_unlock(&file_lock);

    if (!found) {
        send_message(client_socket, "Book not found.\n\n");
    }
}

void view_all_books(int client_socket) {
    char buffer[BUFFER_SIZE], line[100];

    pthread_mutex_lock(&file_lock);
    FILE *file = fopen("books.txt", "r");

    if (!file) {
        perror("File error");
        pthread_mutex_unlock(&file_lock);
        send_message(client_socket, "Failed to open books file.\n\n");
        return;
    }

    send_message(client_socket, "\nList of Books:\n");
    while (fgets(line, sizeof(line), file)) {
        send_message(client_socket, line);
    }

    fclose(file);
    pthread_mutex_unlock(&file_lock);
    send_message(client_socket, "\n");
}

void admin_menu(int client_socket) {
    char buffer[BUFFER_SIZE];
    char choice[3];

    while (1) {
        send_message(client_socket, "\nAdmin Menu:\n1. Add Book\n2. Delete Book\n3. Search Book\n8. View All Books\n10. Logout\nChoice: ");
        memset(buffer, 0, BUFFER_SIZE);
        if (read(client_socket, buffer, BUFFER_SIZE - 1) <= 0) {
            perror("Read error");
            close(client_socket);
            return;
        }
        buffer[strcspn(buffer, "\n")] = '\0';
        strncpy(choice, buffer, 2);
        choice[2] = '\0';

        if (strcmp(choice, "1") == 0) {
            add_book(client_socket);
        } else if (strcmp(choice, "2") == 0) {
            delete_book(client_socket);
        } else if (strcmp(choice, "3") == 0) {
            search_book(client_socket);
        } else if (strcmp(choice, "8") == 0) {
            view_all_books(client_socket);
        } else if (strcmp(choice, "10") == 0) {
            send_message(client_socket, "Logging out...\n\nLogout\n");
            return;
        } else {
            send_message(client_socket, "Invalid choice. Try again.\n\n");
        }
    }
}

void send_message(int client_socket, const char *message) {
    send(client_socket, message, strlen(message), 0);
}
