#pragma once

#include "common.h"

void game_fen(Game* game, char* out);
Result parse_fen(Game* game, const char* fen);

extern const char* FEN_STARTING;
