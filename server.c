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
            char* filename = strtok(buffer + 5, ":");
            char* filedata = strtok(NULL, "");
            if (filename && filedata) {
                FILE* fp = fopen(filename, "wb");
                if (fp) {
                    fwrite(filedata, 1, bytes - (5 + strlen(filename) + 1), fp);
                    fclose(fp);
                    printf("[Client] sent a file. Saved as %s\n", filename);
                } else {
                    perror("Cannot create file");
                }
            }
        } else {
            printf("Client: %s\n", buffer);
        }
    }

    return 0;
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
        msg[strcspn(msg, "\n")] = 0;

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
            char file_buf[BUFFER_SIZE] = "FILE:";
            strcat(file_buf, filename);
            strcat(file_buf, ":");
            int offset = strlen(file_buf);
            int read_bytes = fread(file_buf + offset, 1, BUFFER_SIZE - offset, fp);
            send(client_sock, file_buf, read_bytes + offset, 0);
            fclose(fp);
            printf("[You] File '%s' sent.\n", filename);
        } else {
            send(client_sock, msg, strlen(msg), 0);
        }
    }

    close(client_sock);
    close(server_sock);
    return 0;
}
