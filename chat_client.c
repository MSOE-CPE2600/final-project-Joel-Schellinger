#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

#define SOCKET_PATH "/tmp/chat_socket"
#define BUFFER_SIZE 256

void *receive_messages(void *arg) {
    int client_fd = *(int *)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("%s\n", buffer); // Print the received message with sender info
            fflush(stdout);           // Ensure prompt remains visible
        } else if (bytes_received == 0) {
            printf("Server disconnected.\n");
            exit(0);
        } else {
            perror("recv");
            exit(EXIT_FAILURE);
        }
    }

    return NULL;
}


int main() {
    int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    char username[BUFFER_SIZE];
    do {
        printf("Enter your username: ");
        fgets(username, BUFFER_SIZE, stdin);
        username[strcspn(username, "\n")] = '\0'; // Remove newline character
        if (!strcmp(username, "All")) {
            printf("Cannot make username \'All\'\n");
        }
    }while(!strcmp(username, "All"));

    send(client_fd, username, strlen(username), 0);

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, &client_fd);

    printf("You can now start chatting!\n");

    char buffer[BUFFER_SIZE];
    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline character
        char msg[BUFFER_SIZE];
        send(client_fd, buffer, BUFFER_SIZE, 0);

    }

    close(client_fd);
    return 0;
}
