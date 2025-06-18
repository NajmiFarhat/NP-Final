#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8888
#define BUFFER_SIZE 1024

DWORD WINAPI receive_handler(void* socket_desc) {
    SOCKET sock = *(SOCKET*)socket_desc;
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) break;

        if (strncmp(buffer, "FILE:", 5) == 0) {
            FILE* fp = fopen("received_file.txt", "wb");
            fwrite(buffer + 5, 1, bytes - 5, fp);
            fclose(fp);
            printf("[Server] sent a file. Saved as received_file.txt\n");
        } else {
            printf("Server: %s\n", buffer);
        }
    }
    return 0;
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET sock;
    struct sockaddr_in server;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    // 👇 CHANGE this to your Ubuntu server's actual IP address
    server.sin_addr.s_addr = inet_addr("192.168.56.101");

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("Connect error\n");
        return 1;
    }

    printf("Connected to server.\n");

    CreateThread(NULL, 0, receive_handler, (void*)&sock, 0, NULL);

    char msg[BUFFER_SIZE];
    while (1) {
        fgets(msg, BUFFER_SIZE, stdin);
        msg[strcspn(msg, "\n")] = 0;  // Remove newline

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
            int read_bytes = fread(file_buf + 5, 1, BUFFER_SIZE - 5, fp);
            send(sock, file_buf, read_bytes + 5, 0);
            fclose(fp);
            printf("[You] File '%s' sent.\n", filename);
        } else {
            send(sock, msg, strlen(msg), 0);
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
