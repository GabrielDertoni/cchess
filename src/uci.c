#include <string.h>

#include "common.h"
#include "uci.h"
#include "fen.h"

char const* const uci_command_kind_to_string[] = {
    // gui
    [UCI_INIT]           = "uci",
    [UCI_DEBUG]          = "debug",
    [UCI_ISREADY]        = "isready",
    [UCI_SETOPTION]      = "setoption",
    [UCI_POSITION]       = "position",
    [UCI_GO]             = "go",
    [UCI_STOP]           = "stop",
    [UCI_PONDERHIT]      = "ponderhit",
    [UCI_QUIT]           = "quit",

    // engine
    [UCI_OK]             = "uciok",
    [UCI_READYOK]        = "readyok",
    [UCI_ID]             = "id",
    [UCI_BESTMOVE]       = "bestmove",
    [UCI_COPYPROTECTION] = "copyprotection",
    [UCI_INFO]           = "info",
    [UCI_OPTION]         = "option",

    [UCI_EMPTY]          = "(emtpy)",
};

static const char* delim = " \r\n";

Result uci_parse_command_kind(const char* command, UciCommandKind* out) {
    if (*command == '\0') {
        *out = UCI_EMPTY;
        return RESULT_OK;
    }
    for (int kind = 0; kind < UCI_COUNT; kind++) {
        if (strcmp(command, uci_command_kind_to_string[kind]) == 0) {
            *out = kind;
            return RESULT_OK;
        }
    }
    return ERROR(INVALID_UCI);
}

Result uci_parse_command(char* linebuf, UciCommand* out) {
    char* command = strsep(&linebuf, delim);
    ASSERT_OR(command, INVALID_UCI);
    ASSERT_OK(uci_parse_command_kind(command, &out->kind));

    switch (out->kind) {
        case UCI_OK:
        case UCI_EMPTY:
        case UCI_READYOK:
        case UCI_COPYPROTECTION:
            break;
        case UCI_POSITION: {
            char* s = strsep(&linebuf, delim);
            if (strcmp(s, "startpos") == 0) {
                return parse_fen(&out->position, FEN_STARTING);
            } else if (strcmp(s, "fen") == 0) {
                s = strsep(&linebuf, delim);
                if (!s) return ERROR(INVALID_UCI);
                return parse_fen(&out->position, s);
            }
        }
        case UCI_BESTMOVE: {
            ASSERT_OK(parse_move(strsep(&linebuf, delim), &out->bestmove.move));
            char* s = strsep(&linebuf, delim);
            if (!s) return RESULT_OK;
            ASSERT_OR(strcmp(s, "ponder") == 0, INVALID_UCI);
            out->bestmove.has_ponder = true;
            ASSERT_OK(parse_move(strsep(&linebuf, delim), &out->bestmove.ponder));
            return RESULT_OK;
        }
        case UCI_ID: {
            out->id.name = NULL;
            out->id.author = NULL;
            char* s = strsep(&linebuf, delim);
            ASSERT_OR(s, INVALID_UCI);
            if (strcmp(s, "name") == 0) {
                out->id.name = strsep(&linebuf, delim);
                ASSERT_OR(out->id.name, INVALID_UCI);
            } else if (strcmp(s, "author") == 0) {
                out->id.author = strsep(&linebuf, delim);
                ASSERT_OR(out->id.author, INVALID_UCI);
            }
            return RESULT_OK;
        }
        case UCI_INFO:
            break;
        case UCI_OPTION:
            out->other = linebuf;
            break;
        default:
            return RESULT_OK;
    }
    return RESULT_OK;
}
