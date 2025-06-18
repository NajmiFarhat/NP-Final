// ======== client.c (Windows) - Sends file name + content ========
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
            // Should not receive FILE in this version, but safe guard
            printf("[Server] sent raw file buffer\n");
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

    // 👇 Replace this with your Ubuntu server's IP
    server.sin_addr.s_addr = inet_addr("10.0.2.4");

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("Connect error\n");
        return 1;
    }

    printf("Connected to server.\n");

    CreateThread(NULL, 0, receive_handler, (void*)&sock, 0, NULL);

    char msg[BUFFER_SIZE];
    while (1) {
        printf("> ");
        fgets(msg, BUFFER_SIZE, stdin);
        msg[strcspn(msg, "\n")] = 0;

        if (strcmp(msg, "/file") == 0) {
            char filename[256];
            printf("Enter filename to send (e.g., Test.txt): ");
            fgets(filename, sizeof(filename), stdin);
            filename[strcspn(filename, "\n")] = 0;

            FILE* fp = fopen(filename, "rb");
            if (!fp) {
                perror("Cannot open file");
                continue;
            }

            // Prepare buffer: "FILE:" + 100-byte filename + file content
            char buffer[BUFFER_SIZE];
            memset(buffer, 0, BUFFER_SIZE);
            strcpy(buffer, "FILE:");

            char padded_name[100] = {0};
            strncpy(padded_name, filename, 100);
            memcpy(buffer + 5, padded_name, 100); // offset 5–104 = filename

            int offset = 5 + 100;
            int read_bytes = fread(buffer + offset, 1, BUFFER_SIZE - offset, fp);
            send(sock, buffer, offset + read_bytes, 0);
            fclose(fp);

            printf("[You] Sent file '%s' to server.\n", filename);
        } else {
            send(sock, msg, strlen(msg), 0);
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
