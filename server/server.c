#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080

typedef struct {
    int client_sockets[2];
    char names[2][50];
    char symbols[2];
    char board[9];
    int current_player;
    int game_over;
} GameState;

/* ----------------------------- Utilities ----------------------------- */

void init_board(GameState *game) {
    for (int i = 0; i < 9; i++) game->board[i] = ' ';
}

void broadcast_board(GameState *game) {
    char msg[32];
    sprintf(msg, "BOARD:%c%c%c%c%c%c%c%c%c\n",
        game->board[0], game->board[1], game->board[2],
        game->board[3], game->board[4], game->board[5],
        game->board[6], game->board[7], game->board[8]);
		
		// use to send data to client
    send(game->client_sockets[0], msg, strlen(msg), 0);
    send(game->client_sockets[1], msg, strlen(msg), 0);
}

void send_prompt(GameState *game) {
    int p = game->current_player;
    int o = 1 - p;

    send(game->client_sockets[p], "PROMPT\n", 7, 0);
    send(game->client_sockets[o], "WAIT\n", 5, 0);
}

/* ----------------------------- Logging ----------------------------- */

void log_move(const char *name, char symbol, int pos) {
    // TODO: write move to game_log.txt
}

void log_winner(const char *name, char symbol) {
    // TODO: write winner to winners.txt
}

void log_draw() {
    // TODO: write draw to winners.txt
}

/* ----------------------------- Win/Draw Check ----------------------------- */

int check_winner(GameState *game) {
    int wins[8][3] = {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, // Rows
        {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, // Cols
        {0, 4, 8}, {2, 4, 6}             // Diagonals
    };

    for (int i = 0; i < 8; i++) {
        int a = wins[i][0];
        int b = wins[i][1];
        int c = wins[i][2];

        if (game->board[a] != ' ' &&
            game->board[a] == game->board[b] &&
            game->board[a] == game->board[c]) {
            
            if (game->board[a] == game->symbols[0]) return 0;
            if (game->board[a] == game->symbols[1]) return 1;
        }
    }
    return -1;
}

int check_draw(GameState *game) {
    for (int i = 0; i < 9; i++) {
        if (game->board[i] == ' ') return 0;
    }
    return 1;
}

/* ----------------------------- Main ----------------------------- */

int main() {
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    GameState game;

    /* Create socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    /* Bind */
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind"); exit(1);
    }

    /* Listen */
    if (listen(server_fd, 2) < 0) { perror("listen"); exit(1); }
    printf("Server listening on port %d...\n", PORT);

    /* Accept two clients */
    for (int i = 0; i < 2; i++) {
        printf("Waiting for Player %d...\n", i + 1);
        game.client_sockets[i] = accept(server_fd, (struct sockaddr*)&address, &addrlen);

        if (game.client_sockets[i] < 0) { perror("accept"); exit(1); }

        char buffer[256] = {0};
        read(game.client_sockets[i], buffer, 256);

        if (strncmp(buffer, "NAME:", 5) == 0) {
            strcpy(game.names[i], buffer + 5);
            printf("Player %d name: %s\n", i + 1, game.names[i]);
        }
    }

    /* Assign symbols */
    game.symbols[0] = 'X';
    game.symbols[1] = 'O';

    send(game.client_sockets[0], "SYMBOL:X\n", 9, 0);
    send(game.client_sockets[1], "SYMBOL:O\n", 9, 0);

    /* Initialize game */
    init_board(&game);
    game.current_player = 0;
    game.game_over = 0;

    broadcast_board(&game);
    send_prompt(&game);

    /* Game Loop */
    while (!game.game_over) {
        int p = game.current_player;
        char buffer[256] = {0};

        int val = read(game.client_sockets[p], buffer, 256);
        if (val <= 0) {
            printf("Player %s disconnected!\n", game.names[p]);
            send(game.client_sockets[1 - p], "DISCONNECT\n", 11, 0);
            break;
        }

        if (strncmp(buffer, "MOVE:", 5) == 0) {
            int pos = atoi(buffer + 5) - 1;

            if (pos < 0 || pos > 8 || game.board[pos] != ' ') {
                send(game.client_sockets[p], "INVALID\n", 8, 0);
                continue;
            }

            game.board[pos] = game.symbols[p];
            log_move(game.names[p], game.symbols[p], pos);
            broadcast_board(&game);

            int w = check_winner(&game);
            if (w != -1) {
                char msg[64];
                sprintf(msg, "WINNER:%s\n", game.names[w]);
                send(game.client_sockets[0], msg, strlen(msg), 0);
                send(game.client_sockets[1], msg, strlen(msg), 0);

                log_winner(game.names[w], game.symbols[w]);
                break;
            }

            if (check_draw(&game)) {
                send(game.client_sockets[0], "DRAW\n", 5, 0);
                send(game.client_sockets[1], "DRAW\n", 5, 0);

                log_draw();
                break;
            }

            game.current_player = 1 - game.current_player;
            send_prompt(&game);
        }
    }

    close(game.client_sockets[0]);
    close(game.client_sockets[1]);
    close(server_fd);
    return 0;
}
