#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

typedef struct {
    char my_name[50];
    char my_symbol;
    char board[9];
} ClientState;

void display_board(char b[]) {
    printf("\n=== GAME BOARD ===\n");
    printf(" %c | %c | %c\n", b[0], b[1], b[2]);
    printf("-----------\n");
    printf(" %c | %c | %c\n", b[3], b[4], b[5]);
    printf("-----------\n");
    printf(" %c | %c | %c\n\n", b[6], b[7], b[8]);
}

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    ClientState state;

    /* Create socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); exit(1); }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Connected to server!\n");

    /* Send name */
    printf("Enter your name: ");
    fgets(state.my_name, 50, stdin);
    state.my_name[strcspn(state.my_name, "\n")] = '\0';

    char msg[80];
    sprintf(msg, "NAME:%s\n", state.my_name);
    send(sock, msg, strlen(msg), 0);

    /* Initialize local board */
    for (int i = 0; i < 9; i++) state.board[i] = ' ';

    /* Game Loop */
    char buffer[1024] = {0};
    int len = 0;

    while (1) {
        int val = read(sock, buffer + len, 1024 - len - 1);
        if (val <= 0) {
            printf("Server disconnected.\n");
            break;
        }

        len += val;
        buffer[len] = '\0';

        char *newline;
        while ((newline = strchr(buffer, '\n')) != NULL) {
            *newline = '\0';
            char *msg_ptr = buffer;

            // Process message
            if (strncmp(msg_ptr, "SYMBOL:", 7) == 0) {
                state.my_symbol = msg_ptr[7];
                printf("Your symbol: %c\n", state.my_symbol);
            }
            else if (strncmp(msg_ptr, "BOARD:", 6) == 0) {
                memcpy(state.board, msg_ptr + 6, 9);
                display_board(state.board);
            }
            else if (strcmp(msg_ptr, "PROMPT") == 0) {
                printf("Your turn! Enter position (1-9): ");
                
                char input[64];
                if (fgets(input, sizeof(input), stdin) != NULL) {
                    int pos = atoi(input);
                    char sendbuf[64];
                    sprintf(sendbuf, "MOVE:%d\n", pos);
                    send(sock, sendbuf, strlen(sendbuf), 0);
                }
            }
            else if (strcmp(msg_ptr, "WAIT") == 0) {
                printf("Waiting for opponent...\n");
            }
            else if (strcmp(msg_ptr, "INVALID") == 0) {
                printf("Invalid move! Try again.\n");
                printf("Enter position (1-9): ");
                
                char input[64];
                if (fgets(input, sizeof(input), stdin) != NULL) {
                    int pos = atoi(input);
                    char sendbuf[64];
                    sprintf(sendbuf, "MOVE:%d\n", pos);
                    send(sock, sendbuf, strlen(sendbuf), 0);
                }
            }
            else if (strncmp(msg_ptr, "WINNER:", 7) == 0) {
                printf("\n=== GAME OVER ===\n");
                printf("Winner: %s\n", msg_ptr + 7);

                if (strcmp(msg_ptr + 7, state.my_name) == 0)
                    printf("🎉 YOU WIN! 🎉\n");
                else
                    printf("You lost.\n");

                printf("Starting next game in 3 seconds...\n");
                sleep(3);
                // Continue loop to receive new BOARD
            }
            else if (strcmp(msg_ptr, "DRAW") == 0) {
                printf("\n=== GAME OVER ===\nDraw!\n");
                printf("Starting next game in 3 seconds...\n");
                sleep(3);
                // Continue loop
            }
            else if (strcmp(msg_ptr, "DISCONNECT") == 0) {
                printf("Opponent disconnected.\n");
                goto end_game;
            }

            // Move remaining data to start
            int msg_len = (newline - buffer) + 1;
            int remaining = len - msg_len;
            memmove(buffer, newline + 1, remaining);
            len = remaining;
            buffer[len] = '\0';
        }
    }

end_game:

    close(sock);
    return 0;
}
