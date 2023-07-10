#pragma once

#include "common.h"

#define UCI_COUNT UCI_EMPTY
// source: http://page.mi.fu-berlin.de/block/uci.htm
typedef enum {
    // gui
    UCI_INIT = 0,
    UCI_DEBUG,
    UCI_ISREADY,
    UCI_SETOPTION,
    UCI_POSITION,
    UCI_GO,
    UCI_STOP,
    UCI_PONDERHIT,
    UCI_QUIT,

    // engine
    UCI_OK,
    UCI_READYOK,
    UCI_ID,
    UCI_BESTMOVE,
    UCI_COPYPROTECTION,
    UCI_INFO,
    UCI_OPTION,

    UCI_EMPTY,
} UciCommandKind;

typedef struct {
    char *name;
    char *author;
} UciId;

typedef struct {
    Move move;
    bool has_ponder;
    Move ponder;
} UciBestMove;

typedef struct {
    UciCommandKind kind;
    union {
        UciId id;
        UciBestMove bestmove;
        Game position; // FEN string
        char* other;
    };
} UciCommand;

extern const char* const uci_command_kind_to_string[];

Result uci_parse_command(char* linebuf, UciCommand* out);
