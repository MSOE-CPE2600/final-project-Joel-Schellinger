#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/chat_socket"
#define BUFFER_SIZE 256
#define MAX_CLIENTS 10

typedef struct {
    int fd;
    char username[BUFFER_SIZE];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    char username[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];

    // Receive the username
    recv(client_fd, username, BUFFER_SIZE, 0);
    username[strcspn(username, "\n")] = '\0'; // Remove newline character

    // Add client to the list
    pthread_mutex_lock(&client_mutex);
    clients[client_count].fd = client_fd;
    strncpy(clients[client_count].username, username, BUFFER_SIZE);
    client_count++;
    pthread_mutex_unlock(&client_mutex);

    printf("%s has joined the chat.\n", username);

    while (1) {
        int bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';

            // Parse target username and message
            char *target_username = strtok(buffer, " ");
            char *message = strtok(NULL, "");

            if (target_username && message) {
                // Relay message to the target client
                pthread_mutex_lock(&client_mutex);
                for (int i = 0; i < client_count; i++) {
                    if (strcmp(clients[i].username, target_username) == 0 || !strcmp(target_username, "All")
                    && strcmp(clients[i].username, username)) {
                        char message_with_sender[2*BUFFER_SIZE];
                        snprintf(message_with_sender, 2*BUFFER_SIZE, "[%s]: %s\n", username, message);
                        send(clients[i].fd, message_with_sender, strlen(message_with_sender), 0);
                    }
                }
                pthread_mutex_unlock(&client_mutex);
            }
        } else if (bytes_received == 0) {
            printf("%s has disconnected.\n", username);
            close(client_fd);

            // Remove client from the list
            pthread_mutex_lock(&client_mutex);
            for (int i = 0; i < client_count; i++) {
                if (clients[i].fd == client_fd) {
                    clients[i] = clients[client_count - 1];
                    client_count--;
                    break;
                }
            }
            pthread_mutex_unlock(&client_mutex);
            break;
        } else {
            perror("recv");
            break;
        }
    }

    return NULL;
}

int main() {
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    unlink(SOCKET_PATH); // Remove existing socket file
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Chat server started. Waiting for clients...\n");

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, &client_fd);
        pthread_detach(thread);
    }

    close(server_fd);
    return 0;
}
