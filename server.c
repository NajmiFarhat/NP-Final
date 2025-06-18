#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 8888
#define BUFFER_SIZE 1024

void* receive_handler(void* socket_desc) {
    int client_sock = *(int*)socket_desc;
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_sock, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) break;

        if (strncmp(buffer, "FILE:", 5) == 0) {
            // Extract filename (fixed 100-byte field after "FILE:")
            char filename[101] = {0}; // +1 for null terminator
            memcpy(filename, buffer + 5, 100); // safe copy

            // Open file using extracted filename
            FILE* fp = fopen(filename, "wb");
            if (!fp) {
                perror("Cannot open file");
                continue;
            }

            // Write the remaining bytes as file content
            fwrite(buffer + 5 + 100, 1, bytes - 105, fp);
            fclose(fp);

            printf("[Client] sent a file. Saved as '%s'\n", filename);
        } else {
            printf("Client: %s\n", buffer);
        }
    }

    return NULL;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 1);

    printf("[Server] Waiting for connection...\n");
    client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
    printf("[Server] Client connected.\n");

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_handler, (void*)&client_sock);

    char msg[BUFFER_SIZE];
    while (1) {
        fgets(msg, BUFFER_SIZE, stdin);
        msg[strcspn(msg, "\n")] = 0; // remove newline

        if (strcmp(msg, "/file") == 0) {
            char filename[256];
            printf("Enter filename to send: ");
            fgets(filename, sizeof(filename), stdin);
            filename[strcspn(filename, "\n")] = 0;

            FILE* fp = fopen(filename, "rb");
            if (!fp) {
                perror("Cannot open file");
                continue;
            }

            // Prepare file buffer
            char file_buf[BUFFER_SIZE];
            memset(file_buf, 0, BUFFER_SIZE);
            strcpy(file_buf, "FILE:");

            char padded_name[100] = {0};
            strncpy(padded_name, filename, 100);
            memcpy(file_buf + 5, padded_name, 100);

            int offset = 5 + 100;
            int read_bytes = fread(file_buf + offset, 1, BUFFER_SIZE - offset, fp);
            send(client_sock, file_buf, offset + read_bytes, 0);
            fclose(fp);

            printf("[You] Sent file '%s' to client.\n", filename);
        } else {
            send(client_sock, msg, strlen(msg), 0);
        }
    }

    close(client_sock);
    close(server_sock);
    return 0;
}
