#pragma once

#include "common.h"

MoveHistory make_move(Game* game, Move move);

void unmake_move(Game* game, MoveHistory hist);

int valid_piece_moves(Position position, Piece piece, Game* game, Move moves[]);

int all_valid_moves(Game* game, Move moves[]);

bool is_position_attacked_by_moves(Position pos, Move moves[], int nmoves);

bool check_move(Move move, Game* game, Move valid_moves[], int nmoves);

bool is_move_valid(Move move, Game* game);
