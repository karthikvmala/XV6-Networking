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
        if (board[i][0] != 0 && board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ')
            return (board[i][0] == 1) ? 0 : 1; 
        if (board[0][i] != 0 && board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' ')
            return (board[0][i] == 1) ? 0 : 1;
    }
    if (board[0][0] != 0 && board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ')
        return (board[0][0] == 1) ? 0 : 1;
    if (board[0][2] != 0 && board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' ')
        return (board[0][2] == 1) ? 0 : 1;
    
    return -1; 
}

bool handle_board(int x, int y, int player, int turn) {
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
        send(player, buffer, strlen(buffer), 0);
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

int* handle_rematch(int client1_fd, int client2_fd) {
    static int rematch[2] = {0, 0}; // Static array to hold responses

    memset(buffer, 0, BUFFER_SIZE);
    snprintf(buffer, BUFFER_SIZE, "Do you want to play again? (yes/no)\n");
    send(client1_fd, buffer, strlen(buffer), 0);
    send(client2_fd, buffer, strlen(buffer), 0);

    memset(buffer, 0, BUFFER_SIZE);
    if (recv(client1_fd, buffer, BUFFER_SIZE, 0) > 0) {
        if (strncmp(buffer, "yes", 3) == 0) {
            rematch[0] = 1;
        } else {
            rematch[0] = 0;
        }
    }

    memset(buffer, 0, BUFFER_SIZE);
    if (recv(client2_fd, buffer, BUFFER_SIZE, 0) > 0) {
        if (strncmp(buffer, "yes", 3) == 0) {
            rematch[1] = 1;
        } else {
            rematch[1] = 0;
        }
    }

    return rematch;
}

void print_board(int player) {
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
    send(player, buffer, strlen(buffer), 0);
}

void ask(int client1_fd, int turn) {
    memset(buffer, 0, BUFFER_SIZE);
    snprintf(buffer, BUFFER_SIZE, "Your turn to play. Your symbol is %s.\nEnter row col (1 indexed) or enter \"exit\" to leave:\n", symbol[turn]);
    send(client1_fd, buffer, strlen(buffer), 0);
    memset(buffer, 0, BUFFER_SIZE);
    if (recv(client1_fd, buffer, BUFFER_SIZE, 0) <= 0) {
    perror("Receive error from client 1");
    return;
    }
}

int main() {
    int server_fd, client1_fd, client2_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 2) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    // Accept the first client
    if ((client1_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Accept error");
        exit(EXIT_FAILURE);
    }
    printf("Client 1 connected.\n");
    snprintf(buffer, BUFFER_SIZE, "You are client 1.\n");
    send(client1_fd, buffer, strlen(buffer), 0);

    // Accept the second client
    if ((client2_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Accept error");
        exit(EXIT_FAILURE);
    }
    printf("Client 2 connected.\n");
    snprintf(buffer, BUFFER_SIZE, "You are client 2.\n");
    send(client2_fd, buffer, strlen(buffer), 0);

    snprintf(buffer, BUFFER_SIZE, "Both clients are now connected. Client 1's turn.\n");
    send(client1_fd, buffer, strlen(buffer), 0);
    send(client2_fd, buffer, strlen(buffer), 0);

    int turn = 0;
    while (1) {
        int row, col;
        print_board(client1_fd);
        print_board(client2_fd);
        int winner = checkWinner();
        int draw = checkDraw();

        if (winner != -1 || draw != -1) {
            memset(buffer, 0, BUFFER_SIZE);
            if (draw == 1) {
                snprintf(buffer, BUFFER_SIZE, "It's a draw!\n");
                send(client1_fd, buffer, strlen(buffer), 0);
                send(client2_fd, buffer, strlen(buffer), 0);
            } else {
                snprintf(buffer, BUFFER_SIZE, "Player %d wins!\n", winner + 1);
                send(client1_fd, buffer, strlen(buffer), 0);
                send(client2_fd, buffer, strlen(buffer), 0);
            }

            int* rematch = handle_rematch(client1_fd, client2_fd);

            if (rematch[0] == 1 && rematch[1] == 1) {
                initboard();
                snprintf(buffer, BUFFER_SIZE, "Rematch is started\n");
                send(client1_fd, buffer, strlen(buffer), 0);
                send(client2_fd, buffer, strlen(buffer), 0);
                turn = 0;
                print_board(client1_fd);
            } else if (rematch[0] == 1 && rematch[1] == 0) {
                memset(buffer, 0, BUFFER_SIZE);
                snprintf(buffer, BUFFER_SIZE, "Client 2 does not want to play again. Good bye!\n");
                send(client1_fd, buffer, strlen(buffer), 0);
                memset(buffer, 0, BUFFER_SIZE);
                snprintf(buffer, BUFFER_SIZE, "Good bye!\n");
                send(client2_fd, buffer, strlen(buffer), 0);
                break;
            } else if (rematch[0] == 0 && rematch[1] == 1) {
                memset(buffer, 0, BUFFER_SIZE);
                snprintf(buffer, BUFFER_SIZE, "Client 1 does not want to play again. Good bye!\n");
                send(client2_fd, buffer, strlen(buffer), 0);
                memset(buffer, 0, BUFFER_SIZE);
                snprintf(buffer, BUFFER_SIZE, "Good bye!\n");
                send(client1_fd, buffer, strlen(buffer), 0);
                break;
            } else {
                memset(buffer, 0, BUFFER_SIZE);
                snprintf(buffer, BUFFER_SIZE, "Good bye!\n");
                send(client2_fd, buffer, strlen(buffer), 0);
                memset(buffer, 0, BUFFER_SIZE);
                snprintf(buffer, BUFFER_SIZE, "Good bye!\n");
                send(client1_fd, buffer, strlen(buffer), 0);
                break;
            }
        }

        if (turn == 0) {
            bool check = false;
            bool failed = false;
            while (check == false) {
                if (failed)
                    print_board(client1_fd); 
                ask(client1_fd, turn);
                sscanf(buffer, "%d %d", &row, &col);
                row--; col--;
                if(handle_board(row, col, client1_fd, turn)) {
                    check = true;
                    failed = false;
                } else {
                    failed = true;
                }
            }
        } else {
            bool check = false;
            bool failed = false;
            while (check == false) {
                if (failed) 
                    print_board(client2_fd);    
                ask(client2_fd, turn);
                sscanf(buffer, "%d %d", &row, &col);
                row--; col--;
                if(handle_board(row, col, client2_fd, turn)) {
                    check = true;
                    failed = false;
                } else {
                    failed = true;
                }
            }
        }

        turn ^= 1;
    }
    close(server_fd);
    return 0;
}
