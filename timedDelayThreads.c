/*
Name: Aneesa Tariq
Course: CPAN226
Lab 4 - Multithreading and Network Concurrency
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#define sleep(x) Sleep(1000 * (x))
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

#define PORT 8080

typedef struct {
    SOCKET client_socket;
    int client_id;
} client_info;

void close_socket_portable(SOCKET sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

void* handle_client(void* arg) {
    client_info* info = (client_info*)arg;
    SOCKET client_socket = info->client_socket;
    int client_id = info->client_id;

    char *message =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Hello Client!";

    printf("[Server] Handling client %d...\n", client_id);
    printf("[Server] Processing request for 5 seconds...\n");
    sleep(5);

    send(client_socket, message, (int)strlen(message), 0);
    printf("[Server] Response sent to client %d. Closing connection.\n", client_id);

    close_socket_portable(client_socket);
    free(info);
    return NULL;
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }
#endif

    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_count = 0;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Socket creation failed.\n");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed.\n");
        close_socket_portable(server_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (listen(server_socket, 5) == SOCKET_ERROR) {
        printf("Listen failed.\n");
        close_socket_portable(server_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);
    printf("NOTE: This server is MULTITHREADED. It can handle multiple clients at the same time!\n\n");

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);

        if (client_socket != INVALID_SOCKET) {
            client_count++;

            client_info* info = malloc(sizeof(client_info));
            if (info == NULL) {
                printf("Memory allocation failed.\n");
                close_socket_portable(client_socket);
                continue;
            }

            info->client_socket = client_socket;
            info->client_id = client_count;

            pthread_t tid;
            if (pthread_create(&tid, NULL, handle_client, info) != 0) {
                printf("Thread creation failed for client %d.\n", client_count);
                close_socket_portable(client_socket);
                free(info);
                continue;
            }

            pthread_detach(tid);
        }
    }

    close_socket_portable(server_socket);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}