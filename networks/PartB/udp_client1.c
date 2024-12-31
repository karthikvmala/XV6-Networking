#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdbool.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define CHUNK_SIZE 4
bool first = true;

// Define a structure for messages
typedef struct {
    int seq_no;          // Sequence number of the chunk
    char data[BUFFER_SIZE];  // Buffer for message data
    int total_chunks;    // Total number of chunks being sent
    int length;
} Message;

Message final[100];

typedef struct {
    int ack;
} Ack;

int set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set the socket to non-blocking mode
    if (set_nonblocking(sockfd) < 0) {
        perror("Failed to set non-blocking mode");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // Create a message struct
    
    Message message;

    while (1) {
        // Clear the message data
        memset(message.data, 0, BUFFER_SIZE);
        message.length = 0;
        socklen_t addr_len = sizeof(server_addr);
        
        // Try to receive a message from the server
        int recv_len = recvfrom(sockfd, &message, sizeof(message), 0, (struct sockaddr *)&server_addr, &addr_len);
        
        if (recv_len > 0) {
            message.length = recv_len;  // Store the length of the received message
            message.data[message.length] = '\0';  // Null-terminate the received string
            printf("Server: %s\n", message.data);
            final[message.seq_no] = message;
            
            static char complete_message[BUFFER_SIZE * 100];  // Assuming a maximum of 100 chunks
            static int received_chunks = 0;

            final[message.seq_no] = message;
            received_chunks++;

            // Check if all chunks have been received
            if (received_chunks == message.total_chunks) {
                // Concatenate all chunks to form the complete message
                memset(complete_message, 0, sizeof(complete_message));
                for (int i = 0; i < message.total_chunks; i++) {
                    strcat(complete_message, final[i].data);
                }
                printf("Complete message received: %s\n", complete_message);

                // Reset for the next message
                received_chunks = 0;
                memset(final, 0, sizeof(final));

                // Get user input to send to the server

                printf("Enter message to server: ");
                fgets(message.data, BUFFER_SIZE, stdin);
                message.length = strlen(message.data);
                message.data[message.length - 1] = '\0';  // Replace newline with null terminator

                int total_chunks = (message.length + CHUNK_SIZE - 1) / CHUNK_SIZE;
                message.total_chunks = total_chunks;
                printf("Total chunks: %d\n", total_chunks);
                for (int i = 0; i < total_chunks; i++) {
                    Message chunk_message;
                    chunk_message.seq_no = i;
                    chunk_message.total_chunks = message.total_chunks;
                    int start_index = i * CHUNK_SIZE;
                    int end_index = start_index + CHUNK_SIZE;
                    if (end_index > message.length) {
                        end_index = message.length;
                    }
                    chunk_message.length = end_index - start_index;
                    memcpy(chunk_message.data, &message.data[start_index], chunk_message.length);
                    chunk_message.data[chunk_message.length] = '\0';  // Null-terminate the chunk data

                    // Send the chunk to the server
                    sendto(sockfd, &chunk_message, sizeof(chunk_message), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                    printf("Chunk %d sent to server\n", i);

                }
                printf("Message sent to server\n");

            }
            // Get user input to send to the server
            // printf("Enter message to server: ");
            // fgets(message.data, BUFFER_SIZE, stdin);
            // message.length = strlen(message.data);
            // message.data[message.length - 1] = '\0';  // Replace newline with null terminator

            // // Send message to server
            // sendto(sockfd, message.data, message.length, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
            // printf("Message sent to server\n");
        }

        if (first) {
            first = false;
            // Get user input to send to the server
            printf("Enter message to server: ");
            fgets(message.data, BUFFER_SIZE, stdin);
            message.length = strlen(message.data);
            message.data[message.length - 1] = '\0';  // Replace newline with null terminator

            int total_chunks = (message.length + CHUNK_SIZE - 1) / CHUNK_SIZE;
            message.total_chunks = total_chunks;
            printf("Total chunks: %d\n", total_chunks);
            for (int i = 0; i < total_chunks; i++) {
                Message chunk_message;
                chunk_message.seq_no = i;
                chunk_message.total_chunks = message.total_chunks;
                int start_index = i * CHUNK_SIZE;
                int end_index = start_index + CHUNK_SIZE;
                if (end_index > message.length) {
                    end_index = message.length;
                }
                chunk_message.length = end_index - start_index;
                memcpy(chunk_message.data, &message.data[start_index], chunk_message.length);
                chunk_message.data[chunk_message.length] = '\0';  // Null-terminate the chunk data

                // Send the chunk to the server
                sendto(sockfd, &chunk_message, sizeof(chunk_message), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                printf("Chunk %d sent to server\n", i);

                

            }
            printf("Message sent to server\n");


        }
        // Sleep for a while to avoid busy waiting
        usleep(100000);  // 100 ms
    }

    // Close the socket
    close(sockfd);

    return 0;
}
