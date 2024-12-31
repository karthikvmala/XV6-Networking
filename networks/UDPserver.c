#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define PORT 1234
#define BUFFER_SIZE 1024
int board[3][3];
char symbol[2][2] = {{'X'}, {'O'}};
char buffer[BUFFER_SIZE];

void initboard() {
    memset(board, 0, sizeof(board));
}

int checkWinner() {
    for (int i = 0; i < 2; i++) {
        if (board[i][0] != 0 && board[i][0] == board[i][1] && board[i][1] == board[i][2])
            return (board[i][0] == 1) ? 0 : 1; 
        if (board[0][i] != 0 && board[0][i] == board[1][i] && board[1][i] == board[2][i])
            return (board[0][i] == 1) ? 0 : 1;
    }
    if (board[0][0] != 0 && board[0][0] == board[1][1] && board[1][1] == board[2][2])
        return (board[0][0] == 1) ? 0 : 1;
    if (board[0][2] != 0 && board[0][2] == board[1][1] && board[1][1] == board[2][0])
        return (board[0][2] == 1) ? 0 : 1;
    
    return -1; 
}

bool handle_board(int x, int y, struct sockaddr_in *client_addr, socklen_t addr_len, int sockfd, int turn) {
    if (x < 3 && x >= 0 && y < 3 && y >= 0 && board[x][y] == 0) {
        if (turn == 0) {
            board[x][y] = 1;
        } else {
            board[x][y] = 2;
        }
        return true;
    } else {
        memset(buffer, 0, BUFFER_SIZE);
        snprintf(buffer, BUFFER_SIZE, "Invalid move. Try again.\n");
        sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)client_addr, addr_len);
        return false;
    }
}

int checkDraw() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i][j] == 0) {
                return -1;
            }
        }
    }
    return 1;
}

int* handle_rematch(struct sockaddr_in *client1_addr, struct sockaddr_in *client2_addr, socklen_t addr_len, int sockfd) {
    static int rematch[2] = {0, 0}; // Static array to hold responses

    memset(buffer, 0, BUFFER_SIZE);
    snprintf(buffer, BUFFER_SIZE, "Do you want to play again? (yes/no)\n");
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)client1_addr, addr_len);
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)client2_addr, addr_len);

    memset(buffer, 0, BUFFER_SIZE);
    recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)client1_addr, &addr_len);
    if (strncmp(buffer, "yes", 3) == 0) {
        rematch[0] = 1;
    } else {
        rematch[0] = 0;
    }

    memset(buffer, 0, BUFFER_SIZE);
    recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)client2_addr, &addr_len);
    if (strncmp(buffer, "yes", 3) == 0) {
        rematch[1] = 1;
    } else {
        rematch[1] = 0;
    }

    return rematch;
}

void print_board(struct sockaddr_in *client_addr, socklen_t addr_len, int sockfd) {
    memset(buffer, 0, BUFFER_SIZE);
    int offset = 0;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i][j] == 1) {
                offset += snprintf(buffer + offset, BUFFER_SIZE - offset, "X ");
            } else if (board[i][j] == 2) {
                offset += snprintf(buffer + offset, BUFFER_SIZE - offset, "O ");
            } else {
                offset += snprintf(buffer + offset, BUFFER_SIZE - offset, ". ");
            }
        }
        offset += snprintf(buffer + offset, BUFFER_SIZE - offset, "\n");
    }
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)client_addr, addr_len);
}

void ask(struct sockaddr_in *client_addr, socklen_t addr_len, int sockfd, int turn) {
    memset(buffer, 0, BUFFER_SIZE);
    snprintf(buffer, BUFFER_SIZE, "Your turn to play. Your symbol is %s.\nEnter row col (1 indexed) or enter \"exit\" to leave:\n", symbol[turn]);
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)client_addr, addr_len);
    memset(buffer, 0, BUFFER_SIZE);
    recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)client_addr, &addr_len);
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client1_addr, client2_addr;
    socklen_t addr_len = sizeof(server_addr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    // Receive connection from client 1
    recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client1_addr, &addr_len);
    printf("Client 1 connected.\n");
    snprintf(buffer, BUFFER_SIZE, "You are client 1.\n");
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client1_addr, addr_len);

    // Receive connection from client 2
    recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client2_addr, &addr_len);
    printf("Client 2 connected.\n");
    snprintf(buffer, BUFFER_SIZE, "You are client 2.\n");
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client2_addr, addr_len);

    snprintf(buffer, BUFFER_SIZE, "Both clients are now connected. Client 1's turn.\n");
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client1_addr, addr_len);
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client2_addr, addr_len);

    int turn = 0;
    while (1) {
        int row, col;
        print_board(&client1_addr, addr_len, sockfd);
        print_board(&client2_addr, addr_len, sockfd);
        int winner = checkWinner();
        int draw = checkDraw();

        if (winner != -1 || draw != -1) {
            memset(buffer, 0, BUFFER_SIZE);
            if (draw == 1) {
                snprintf(buffer, BUFFER_SIZE, "It's a draw!\n");
                sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client1_addr, addr_len);
                sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client2_addr, addr_len);
            } else {
                snprintf(buffer, BUFFER_SIZE, "Player %d wins!\n", winner + 1);
                sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client1_addr, addr_len);
                sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client2_addr, addr_len);
            }

            int* rematch = handle_rematch(&client1_addr, &client2_addr, addr_len, sockfd);

            if (rematch[0] == 1 && rematch[1] == 1) {
                initboard();
                snprintf(buffer, BUFFER_SIZE, "Rematch is started\n");
                sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client1_addr, addr_len);
                sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client2_addr, addr_len);
                turn = 0;
                print_board(&client1_addr, addr_len, sockfd);
            } else if (rematch[0] == 1 && rematch[1] == 0) {
                memset(buffer, 0, BUFFER_SIZE);
                snprintf(buffer, BUFFER_SIZE, "Client 2 does not want to play again. Good bye!\n");
                sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client1_addr, addr_len);
                memset(buffer, 0, BUFFER_SIZE);
                snprintf(buffer, BUFFER_SIZE, "Good bye!\n");
                sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client2_addr, addr_len);
                break;
            } else if (rematch[0] == 0 && rematch[1] == 1) {
                memset(buffer, 0, BUFFER_SIZE);
                snprintf(buffer, BUFFER_SIZE, "Client 1 does not want to play again. Good bye!\n");
                sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client2_addr, addr_len);
                memset(buffer, 0, BUFFER_SIZE);
                snprintf(buffer, BUFFER_SIZE, "Good bye!\n");
                sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client1_addr, addr_len);
                break;
            } else {
                memset(buffer, 0, BUFFER_SIZE);
                snprintf(buffer, BUFFER_SIZE, "Good bye!\n");
                sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client2_addr, addr_len);
                memset(buffer, 0, BUFFER_SIZE);
                snprintf(buffer, BUFFER_SIZE, "Good bye!\n");
                sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client1_addr, addr_len);
                break;
            }
        }

        if (turn == 0) {
            bool check = false;
            bool failed = false;
            while (!check) {
                if (failed) 
                    print_board(&client1_addr, addr_len, sockfd);
                ask(&client1_addr, addr_len, sockfd, turn);
                sscanf(buffer, "%d %d", &row, &col);
                row--; col--;
                if (handle_board(row, col, &client1_addr, addr_len, sockfd, turn)) {
                    check = true;
                    failed = false;
                } else {
                    failed = true;
                }
            }
        } else {
            bool check = false;
            bool failed = false;
            while (!check) {
                if (failed)
                    print_board(&client2_addr, addr_len, sockfd);
                ask(&client2_addr, addr_len, sockfd, turn);
                sscanf(buffer, "%d %d", &row, &col);
                row--; col--;
                if (handle_board(row, col, &client2_addr, addr_len, sockfd, turn)) {
                    check = true;
                    failed = false;
                } else {
                    failed = true;
                }
            }
        }
        turn ^= 1;
    }
    close(sockfd);
    return 0;
}
