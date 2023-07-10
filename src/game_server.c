#include <string.h>

#include "game_server.h"
#include "common.h"
#include "uci.h"
#include "fen.h"
#include "moves.h"

Result player_init(Player* this) {
    this->linebuf = NULL;
    this->linecap = 0;
    this->in = NULL;
    this->out = NULL;
    return RESULT_OK;
}

void player_deinit(Player* this) {
    if (this->linebuf) free(this->linebuf);
    if (this->in) fclose(this->in);
    if (this->out) fclose(this->out);
}

Result game_server_init_from_fen(GameServer* this, const char* fen) {
    this->ai_color = COLOR_BLACK;
    ASSERT_OK(parse_fen(&this->game, fen));
    this->is_startpos = true;
    this->is_done = false;
    ASSERT_OK(player_init(&this->players[COLOR_WHITE]));
    ASSERT_OK(player_init(&this->players[COLOR_BLACK]));
    return RESULT_OK;
}

void game_server_deinit(GameServer* this) {
    player_deinit(&this->players[COLOR_WHITE]);
    player_deinit(&this->players[COLOR_BLACK]);
}

Result player_uci_read(Player* player, UciCommand* cmd) {
    ssize_t len = getline(&player->linebuf, &player->linecap, player->in);
    ASSERT_OR(len != EOF, LIBC);
    // Remove newline char
    while (strchr("\r\n", player->linebuf[len-1])) len--;
    player->linebuf[len] = '\0';
    return uci_parse_command(player->linebuf, cmd);
}

Result player_uci_read_until_kind(Player* player, UciCommandKind kind, UciCommand* out) {
    // fprintf(stderr, "[info]: waiting %s\n", uci_command_kind_to_string[kind]);
    while (1) {
        UciCommand cmd;
        Result res = player_uci_read(player, &cmd);
        if (res != RESULT_OK) {
            if (feof(player->in)) return RESULT_ERR_EOF;
            fprintf(
                stderr,
                "[error]: invalid UCI command, expecting '%s'\n",
                uci_command_kind_to_string[kind]
            );
            fprintf(stderr, "         * %s\n", get_error_msg(res));
            continue;
        }
        if (cmd.kind == kind) {
            if (out) *out = cmd;
            return RESULT_OK;
        }
    }
}

Result game_server_setup_uci(GameServer* server) {
    for (int i = 0; i < 2; i++) {
        fprintf(stderr, "waiting for player %d to connect\n", i);
        Player* player = &server->players[i];
        ASSERT_OR(fprintf(player->out, "uci\n") >= 0, LIBC);
        ASSERT_OK(player_uci_read_until_kind(player, UCI_OK, NULL));
        ASSERT_OR(fprintf(player->out, "isready\n") >= 0, LIBC);
        ASSERT_OK(player_uci_read_until_kind(player, UCI_READYOK, NULL));
    }
    return RESULT_OK;
}

Result game_server_update(GameServer* server) {
    Game* game = &server->game;
    Player* player = &server->players[game->turn];

    Move possible_moves[MAX_MOVES];
    memset(possible_moves, 0, sizeof(possible_moves));
    int count = all_valid_moves(game, possible_moves);
    if (count == 0) {
        server->is_done = true;
        fprintf(stderr, "[info] %s wins\n", opposite(game->turn) == COLOR_WHITE ? "white" : "black");
        return RESULT_OK;
    }

    char fen[MAX_FEN_LENGTH + 1];
    game_fen(game, fen);

    if (server->is_startpos) {
        ASSERT_OR(fprintf(player->out, "position startpos\n") >= 0, LIBC);
        server->is_startpos = false;
    } else {
        ASSERT_OR(fprintf(player->out, "position fen %s\n", fen) >= 0, LIBC);
    }
    ASSERT_OR(fprintf(player->out, "go depth 10\n") >= 0, LIBC);

    UciCommand cmd;
    ASSERT_OK(player_uci_read_until_kind(player, UCI_BESTMOVE, &cmd));
    Move move = cmd.bestmove.move;
    if (!check_move(move, &server->game, possible_moves, count)) {
        fprintf(stderr, "[error] invalid move %.5s\n", (char*)&move);
        return RESULT_OK;
    }
    int history_slot = server->game.fullmove_counter % HISTORY_MAX;
    server->history[history_slot] = make_move(&server->game, move);

    game->turn = opposite(game->turn);
    return RESULT_OK;
}
