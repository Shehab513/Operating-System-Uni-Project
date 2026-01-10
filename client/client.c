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
    sprintf(msg, "NAME:%s", state.my_name);
    send(sock, msg, strlen(msg), 0);

    /* Initialize local board */
    for (int i = 0; i < 9; i++) state.board[i] = ' ';

    /* Game Loop */
    while (1) {
        char buffer[256] = {0};
        int val = read(sock, buffer, 256);

        if (val <= 0) {
            printf("Server disconnected.\n");
            break;
        }

        if (strncmp(buffer, "SYMBOL:", 7) == 0) {
            state.my_symbol = buffer[7];
            printf("Your symbol: %c\n", state.my_symbol);
        }
        else if (strncmp(buffer, "BOARD:", 6) == 0) {
            memcpy(state.board, buffer + 6, 9);
            display_board(state.board);
        }
        else if (strcmp(buffer, "PROMPT") == 0) {
            int pos;
            printf("Your turn! Enter position (1-9): ");
            scanf("%d", &pos);

            char sendbuf[20];
            sprintf(sendbuf, "MOVE:%d", pos);
            send(sock, sendbuf, strlen(sendbuf), 0);
        }
        else if (strcmp(buffer, "WAIT") == 0) {
            printf("Waiting for opponent...\n");
        }
        else if (strcmp(buffer, "INVALID") == 0) {
            printf("Invalid move! Try again.\n");
        }
        else if (strncmp(buffer, "WINNER:", 7) == 0) {
            printf("\n=== GAME OVER ===\n");
            printf("Winner: %s\n", buffer + 7);

            if (strcmp(buffer + 7, state.my_name) == 0)
                printf("🎉 YOU WIN! 🎉\n");
            else
                printf("You lost.\n");

            break;
        }
        else if (strcmp(buffer, "DRAW") == 0) {
            printf("\n=== GAME OVER ===\nDraw!\n");
            break;
        }
        else if (strcmp(buffer, "DISCONNECT") == 0) {
            printf("Opponent disconnected.\n");
            break;
        }
    }

    close(sock);
    return 0;
}
