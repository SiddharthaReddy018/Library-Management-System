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
void add_member(int client_socket);
void add_admin(int client_socket);
void delete_member(int client_socket);
void search_member(int client_socket);
void view_all_books(int client_socket);
void view_borrowed_books(int client_socket);
void borrow_book(int client_socket,const char *username);
void return_book(int client_socket,const char *username);
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
            user_index = atoi(stored_user); // Assuming the username is a number
            break;
        }
    }

    fclose(file);
    pthread_mutex_unlock(&file_lock);

    if (authenticated && !is_logged_in[user_index]) {
        is_logged_in[user_index] = true; // Mark the user as logged in
    } else if (authenticated && is_logged_in[user_index]) {
        printf("User already logged in\n");
        authenticated = 0; // User is already logged in
    }

    return authenticated;
}

void *handle_client(void *client_socket) {
    int sock = *(int *)client_socket;
    char buffer[BUFFER_SIZE];
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

    // Send logout message before closing the socket
    send_message(sock, "Logout\n");

    // Mark the user as logged out
    is_logged_in[atoi(username)] = false;

    close(sock); // Close the socket when client logs out
    return NULL;
}
void admin_menu(int client_socket) {
    char buffer[BUFFER_SIZE];
    char choice[3];  // Increased size to handle multi-character choices

    while (1) {
        send_message(client_socket, "\nAdmin Menu:\n1. Add Book\n2. Delete Book\n3. Search Book\n4. Add User\n5. Add Admin\n6. Delete Member\n7. Search Member\n8. View All Books\n9. View Borrowed Books\n10. Logout\nChoice: ");
        memset(buffer, 0, BUFFER_SIZE); // Clear the buffer before reading
        if (read(client_socket, buffer, BUFFER_SIZE - 1) <= 0) {
            perror("Read error");
            close(client_socket);
            return;
        }
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline character
        strncpy(choice, buffer, 2);  // Copy the first two characters to choice
        choice[2] = '\0';  // Null-terminate the string

        // Debugging output to check what was received
        printf("Received choice: %s\n", choice);
        send_message(client_socket, "Received choice\n"); // Send the choice back to client for debugging

        if (strcmp(choice, "1") == 0) {
            add_book(client_socket);
        } else if (strcmp(choice, "2") == 0) {
            delete_book(client_socket);
        } else if (strcmp(choice, "3") == 0) {
            search_book(client_socket);
        } else if (strcmp(choice, "4") == 0) {
            add_member(client_socket);
        } else if (strcmp(choice, "5") == 0) {
            add_admin(client_socket);
        } else if (strcmp(choice, "6") == 0) {
            delete_member(client_socket);
        } else if (strcmp(choice, "7") == 0) {
            search_member(client_socket);
        } else if (strcmp(choice, "8") == 0) {
            view_all_books(client_socket);
        } else if (strcmp(choice, "9") == 0) {
            view_borrowed_books(client_socket);
        } else if (strcmp(choice, "10") == 0) {
            send_message(client_socket, "Logging out...\n\nLogout\n");
            return;
        } else {
            send_message(client_socket, "Invalid choice. Try again.\n\n");
        }
    }
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

    fprintf(file, "%03d,%s,%s,%s\n", atoi(isbn), title, author,"1");
    fclose(file);
    pthread_mutex_unlock(&file_lock);

    snprintf(buffer, sizeof(buffer), "Book '%s' added successfully.\n\n", title);
    send_message(client_socket, buffer);
}

void delete_book(int client_socket) {
    char search_term[50], buffer[BUFFER_SIZE], line[100];
    int found = 0;

    send_message(client_socket, "Enter book ISBN or title to delete: ");
    read(client_socket, search_term, 50);
    search_term[strcspn(search_term, "\n")] = '\0';

    pthread_mutex_lock(&file_lock);
    FILE *file = fopen("books.txt", "r");
    FILE *temp = fopen("temp.txt", "w");

    if (!file || !temp) {
        perror("Could not open file");
        pthread_mutex_unlock(&file_lock);
        send_message(client_socket, "Failed to delete book.\n\n");
        return;
    }

    while (fgets(line, sizeof(line), file)) {
        char record_isbn[4], title[50];
        sscanf(line, "%3[^,],%[^,]", record_isbn, title);
        if (strcmp(record_isbn, search_term) != 0 && strcmp(title, search_term) != 0) {
            fputs(line, temp);
        } else {
            found = 1;
        }
    }

    fclose(file);
    fclose(temp);
    remove("books.txt");
    rename("temp.txt", "books.txt");
    pthread_mutex_unlock(&file_lock);

    if (found) {
        snprintf(buffer, sizeof(buffer), "Book with ISBN or title '%s' deleted successfully.\n\n", search_term);
    } else {
        snprintf(buffer, sizeof(buffer), "Book with ISBN or title '%s' not found.\n\n", search_term);
    }

    send_message(client_socket, buffer);
}

void search_book(int client_socket) {
    char search_term[50], buffer[BUFFER_SIZE], line[100];
    int found = 0;

    send_message(client_socket, "Enter ISBN or title to search for book: ");
    read(client_socket, search_term, 50);
    search_term[strcspn(search_term, "\n")] = '\0';

    FILE *file = fopen("books.txt", "r");

    if (!file) {
        perror("Could not open books.txt");
        send_message(client_socket, "Failed to search for book.\n\n");
        return;
    }

    while (fgets(line, sizeof(line), file)) {
        char record_isbn[4], title[50], author[50];
        sscanf(line, "%3[^,],%[^,],%[^,]", record_isbn, title, author);
        if (strcmp(record_isbn, search_term) == 0 || strcmp(title, search_term) == 0) {
            snprintf(buffer, sizeof(buffer), "Book found: %s, %s, %s\n", record_isbn, title, author);
            send_message(client_socket, buffer);
            found = 1;
        }
    }

    fclose(file);

    if (!found) {
        snprintf(buffer, sizeof(buffer), "Book with ISBN or title '%s' not found.\n\n", search_term);
        send_message(client_socket, buffer);
    }
}

void add_member(int client_socket) {
    char username[50], password[50],role[10]="user", buffer[BUFFER_SIZE];
    
    send_message(client_socket, "Enter new member username: ");
    read(client_socket, username, 50);
    username[strcspn(username, "\n")] = '\0';
    
    send_message(client_socket, "Enter new member password: ");
    read(client_socket, password, 50);
    password[strcspn(password, "\n")] = '\0';
    
    pthread_mutex_lock(&file_lock);
    FILE *file = fopen("users.txt", "a");
        if (!file) {
        perror("Could not open users.txt");
        pthread_mutex_unlock(&file_lock);
        send_message(client_socket, "Failed to add member.\n\n");
        return;
    }
    
    fprintf(file, "%s %s\n", username, password);
    fclose(file);
    pthread_mutex_unlock(&file_lock);
    
    snprintf(buffer, sizeof(buffer), "Member '%s' added successfully.\n\n", username);
    send_message(client_socket, buffer);
}
void add_admin(int client_socket) {
    char adminname[50], password[50], role[10] = "admin", buffer[BUFFER_SIZE];
    
    send_message(client_socket, "Enter new admin username: ");
    if (read(client_socket, adminname, 50) <= 0) {
        perror("Read error");
        close(client_socket);
        return;
    }
    adminname[strcspn(adminname, "\n")] = '\0';
    
    send_message(client_socket, "Enter new admin password: ");
    if (read(client_socket, password, 50) <= 0) {
        perror("Read error");
        close(client_socket);
        return;
    }
    password[strcspn(password, "\n")] = '\0';
    
    pthread_mutex_lock(&file_lock);
    FILE *file = fopen("users.txt", "a");
    if (!file) {
        perror("Could not open users.txt");
        pthread_mutex_unlock(&file_lock);
        send_message(client_socket, "Failed to add admin.\n\n");
        return;
    }
    
    fprintf(file, "%s %s %s\n", adminname, password, role);
    fclose(file);
    pthread_mutex_unlock(&file_lock);
    
    snprintf(buffer, sizeof(buffer), "Admin '%s' added successfully.\n\n", adminname);
    send_message(client_socket, buffer);
}


void delete_member(int client_socket) {
    char username[50], buffer[BUFFER_SIZE], line[100];
    int found = 0;
    
    send_message(client_socket, "Enter member username to delete: ");
    read(client_socket, username, 50);
    username[strcspn(username, "\n")] = '\0';
    
    pthread_mutex_lock(&file_lock);
    FILE *file = fopen("users.txt", "r");
    FILE *temp = fopen("temp.txt", "w");
    
    if (!file || !temp) {
        perror("Could not open file");
        pthread_mutex_unlock(&file_lock);
        send_message(client_socket, "Failed to delete member.\n\n");
        return;
    }
    
    while (fgets(line, sizeof(line), file)) {
        char record_username[50];
        sscanf(line, "%s", record_username);
        if (strcmp(record_username, username) != 0) {
            fputs(line, temp);
        } else {
            found = 1;
        }
    }
    
    fclose(file);
    fclose(temp);
    remove("users.txt");
    rename("temp.txt", "users.txt");
    pthread_mutex_unlock(&file_lock);
    
    if (found) {
        snprintf(buffer, sizeof(buffer), "Member '%s' deleted successfully.\n\n", username);
    } else {
        snprintf(buffer, sizeof(buffer), "Member '%s' not found.\n\n", username);
    }
    
    send_message(client_socket, buffer);
}

void search_member(int client_socket) {
    char username[50], buffer[BUFFER_SIZE], line[100];
    int found = 0;
    
    send_message(client_socket, "Enter username to search for member: ");
    read(client_socket, username, 50);
    username[strcspn(username, "\n")] = '\0';
    FILE *file = fopen("users.txt", "r");
    
    if (!file) {
        perror("Could not open users.txt");
        pthread_mutex_unlock(&file_lock);
        send_message(client_socket, "Failed to search for member.\n\n");
        return;
    }
    
    while (fgets(line, sizeof(line), file)) {
        char record_username[50];
        sscanf(line, "%s", record_username);
        if (strcmp(record_username, username) == 0) {
            snprintf(buffer, sizeof(buffer), "Member '%s' found.\n\n", username);
            send_message(client_socket, buffer);
            found = 1;
            break;
        }
    }
    
    fclose(file);
    
    if (!found) {
        snprintf(buffer, sizeof(buffer), "Member '%s' not found.\n\n", username);
        send_message(client_socket, buffer);
    }
}

void view_all_books(int client_socket) {
    char buffer[BUFFER_SIZE], line[100];
    FILE *file = fopen("books.txt", "r");
    
    if (!file) {
        perror("Could not open books.txt");
        send_message(client_socket, "Failed to retrieve books.\n\n");
        return;
    }
    
    send_message(client_socket, "All Books:\n");
    while (fgets(line, sizeof(line), file)) {
        send_message(client_socket, line);
    }
    
    fclose(file);
}


void view_borrowed_books(int client_socket) {
    char buffer[BUFFER_SIZE], line[100];
    
    pthread_mutex_lock(&file_lock);
    FILE *file = fopen("borrowed_books.txt", "r");
    
    if (!file) {
        perror("Could not open borrowed_books.txt");
        pthread_mutex_unlock(&file_lock);
        send_message(client_socket, "Failed to retrieve borrowed books.\n\n");
        return;
    }
    
    send_message(client_socket, "Borrowed Books:\n");
    while (fgets(line, sizeof(line), file)) {
        send_message(client_socket, line);
    }
    
    fclose(file);
    pthread_mutex_unlock(&file_lock);
}

void borrow_book(int client_socket, const char *username) {
    char search_term[50], buffer[BUFFER_SIZE], line[100];
    int found = 0;
    int available = 0;

    send_message(client_socket, "Enter book ISBN or title to borrow: ");
    read(client_socket, search_term, 50);
    search_term[strcspn(search_term, "\n")] = '\0';


    pthread_mutex_lock(&file_lock);  // Lock the critical section
    // sleep(10);   //for demonstrating the critical section 
    FILE *file = fopen("books.txt", "r+");
    FILE *borrowed_file = fopen("borrowed_books.txt", "a");

    if (!file || !borrowed_file) {
        perror("Could not open file");
        if (file) fclose(file);
        if (borrowed_file) fclose(borrowed_file);
        pthread_mutex_unlock(&file_lock);
        send_message(client_socket, "Failed to borrow book.\n\n");
        return;
    }

    while (fgets(line, sizeof(line), file)) {
        char record_isbn[4], title[50], author[50], available_str[2];
        sscanf(line, "%3[^,],%[^,],%[^,],%s", record_isbn, title, author, available_str);
        available = atoi(available_str);
        if ((strcmp(record_isbn, search_term) == 0 || strcmp(title, search_term) == 0) && available == 1) {
            found = 1;
            fseek(file, -strlen(line), SEEK_CUR);
            fprintf(file, "%03d,%s,%s,0\n", atoi(record_isbn), title, author);
            fprintf(borrowed_file, "%s,%s,%s,%s\n", username, record_isbn, title, author);
            break;
        }
    }

    fclose(file);
    fclose(borrowed_file);
    pthread_mutex_unlock(&file_lock);  

    if (found) {
        snprintf(buffer, sizeof(buffer), "Book with ISBN or title '%s' borrowed by user '%s' successfully.\n\n", search_term, username);
    } else {
        snprintf(buffer, sizeof(buffer), "Book with ISBN or title '%s' not found or already borrowed.\n\n", search_term);
    }

    send_message(client_socket, buffer);
}


void return_book(int client_socket, const char *username) {
    char search_term[50], buffer[BUFFER_SIZE], line[100];
    int found = 0;

    send_message(client_socket, "Enter book ISBN or title to return: ");
    memset(search_term, 0, sizeof(search_term));
    if (read(client_socket, search_term, sizeof(search_term) - 1) <= 0) {
        perror("Read error");
        return;
    }
    search_term[strcspn(search_term, "\n")] = '\0';

    pthread_mutex_lock(&file_lock);
    FILE *file = fopen("books.txt", "r+");
    FILE *borrowed_file = fopen("borrowed_books.txt", "r");
    FILE *temp_borrowed_file = fopen("temp_borrowed.txt", "w");

    if (!file || !borrowed_file || !temp_borrowed_file) {
        perror("Could not open file");
        if (file) fclose(file);
        if (borrowed_file) fclose(borrowed_file);
        if (temp_borrowed_file) fclose(temp_borrowed_file);
        pthread_mutex_unlock(&file_lock);
        send_message(client_socket, "Failed to return book.\n\n");
        return;
    }

    while (fgets(line, sizeof(line), borrowed_file)) {
        char record_username[50], record_isbn[4], title[50], author[50];
        sscanf(line, "%[^,],%3[^,],%[^,],%[^,]", record_username, record_isbn, title, author);
        if ((strcmp(record_isbn, search_term) == 0 || strcmp(title, search_term) == 0) && strcmp(record_username, username) == 0) {
            found = 1;
        } else {
            fputs(line, temp_borrowed_file);
        }
    }

    if (found) {
        while (fgets(line, sizeof(line), file)) {
            char record_isbn[4], title[50], author[50], available[2];
            sscanf(line, "%3[^,],%[^,],%[^,],%s", record_isbn, title, author, available);
            if (strcmp(record_isbn, search_term) == 0 || strcmp(title, search_term) == 0) {
                fseek(file, -strlen(line), SEEK_CUR);
                fprintf(file, "%03d,%s,%s,1\n", atoi(record_isbn), title, author);
                break;
            }
        }
    }

    fclose(file);
    fclose(borrowed_file);
    fclose(temp_borrowed_file);
    remove("borrowed_books.txt");
    rename("temp_borrowed.txt", "borrowed_books.txt");
    pthread_mutex_unlock(&file_lock);

    if (found) {
        snprintf(buffer, sizeof(buffer), "Book with ISBN or title '%s' returned successfully.\n\n", search_term);
    } else {
        snprintf(buffer, sizeof(buffer), "Book with ISBN or title '%s' not found or not borrowed by this user.\n\n", search_term);
    }

    send_message(client_socket, buffer);
}

void user_menu(int client_socket, const char *username) {
    char choice;
    char buffer[BUFFER_SIZE];

    while (1) {
        send_message(client_socket, "\nUser Menu:\n1. Search Book\n2. Borrow Book\n3. Return Book\n4. Logout\nChoice: ");
        memset(buffer, 0, BUFFER_SIZE); // Clear the buffer before reading
        if (read(client_socket, buffer, BUFFER_SIZE - 1) <= 0) {
            perror("Read error");
            close(client_socket);
            return;
        }
        choice = buffer[0];

        switch (choice) {
            case '1':
                search_book(client_socket);
                break;
            case '2':
                borrow_book(client_socket, username);
                break;
            case '3':
                return_book(client_socket, username);
                break;
            case '4':
                send_message(client_socket, "Logging out...\n\nLogout\n");
                return;
            default:
                send_message(client_socket, "Invalid choice. Try again.\n\n");
        }
    }
}




void send_message(int client_socket, const char *message) {
    write(client_socket, message, strlen(message));
}
