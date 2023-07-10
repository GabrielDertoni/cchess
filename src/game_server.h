#pragma once

#include "common.h"
#include "uci.h"

typedef struct {
    char* linebuf;
    size_t linecap;
    FILE* in;
    FILE* out;
} Player;

typedef struct {
    MoveHistory history[HISTORY_MAX];
    PieceColor ai_color;
    Game game;
    bool is_startpos;
    bool is_done;
    Player players[2];
} GameServer;

Result game_server_init_from_fen(GameServer* server, const char* fen);

void game_server_deinit(GameServer* server);

Result game_server_setup_uci(GameServer* server);

Result game_server_update(GameServer* server);
