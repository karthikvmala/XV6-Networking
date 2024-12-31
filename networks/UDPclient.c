#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 1234
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(serv_addr);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // UDP doesn't require connect(), we can directly send messages using sendto()

    // Initial message to notify server of connection (simulates connection for UDP)
    snprintf(buffer, BUFFER_SIZE, "Hello, server!\n");
    sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&serv_addr, addr_len);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int valread = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serv_addr, &addr_len);
        if (valread > 0) {
            buffer[valread] = '\0';
            printf("%s", buffer);
        }

        // Prompt the user if it's their turn or if asked a question
        if (strstr(buffer, "Your turn") != NULL || strstr(buffer, "Invalid move. Try again.") != NULL || strstr(buffer, "yes/no") != NULL) {
            memset(buffer, 0, BUFFER_SIZE);
            fgets(buffer, BUFFER_SIZE, stdin);
            sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&serv_addr, addr_len);
        }

        // Exit the loop if the game is over or user sends "exit"
        if (strstr(buffer, "exit") != NULL || strstr(buffer, "Good bye") != NULL) {
            break;
        }
    }

    close(sock);
    return 0;
}
