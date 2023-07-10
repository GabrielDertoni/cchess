#include <string.h>
#include <errno.h>
#include "common.h"

const char kings_rank[] = {
    [COLOR_WHITE] = '1',
    [COLOR_BLACK] = '8',
};

static const char* const error_msg[] = {
    [RESULT_OK] = "ok",
    [RESULT_ERR_EOF] = "end of file",
    [RESULT_ERR_INVALID_FEN] = "invalid fen",
    [RESULT_ERR_INVALID_UCI] = "invalid uci",
    [RESULT_ERR_INVALID_POSITION] = "invalid position",
    [RESULT_ERR_INVALID_PROMOTION] = "invalid promotion",
    [RESULT_ERR_INVALID_PIECE] = "invalid piece",
    [RESULT_ERR_LIBC] = "libc error",
};

const char* get_error_msg(Result res) {
    if (res == RESULT_ERR_LIBC)
        return strerror(errno);
    else
        return error_msg[res];
}

Square* board_index(Position position, Board* board) {
    if (position.rank < '1' || position.rank > '8' || position.file < 'a' || position.file > 'h')
        return NULL;
    return &board->squares[position.rank - '1'][position.file - 'a'];
}

void swap_squares(Square* a, Square* b) {
    Square tmp = *a;
    *a = *b;
    *b = tmp;
}

PieceColor opposite(PieceColor color) {
    return color == COLOR_WHITE ? COLOR_BLACK : COLOR_WHITE;
}

char piece_letter(Piece piece) {
    char c = piece.kind;
    if (piece.color == COLOR_WHITE)
        c -= 32; // to upper case
    return c;
}

Result piece_from_letter(char letter, Piece* piece) {
    bool is_upper = letter >= 'A' && letter <= 'Z';
    if (is_upper) letter += 32; // to lower case
    piece->color = is_upper ? COLOR_WHITE : COLOR_BLACK;
    ASSERT_OR(strchr(piece_kinds, letter), INVALID_PIECE);
    piece->kind = letter;
    return RESULT_OK;
}

Result parse_position(const char* s, Position* out) {
    ASSERT_OR(s && s[0] >= 'a' && s[0] <= 'h'
                && s[1] >= '0' && s[1] <= '8', INVALID_POSITION);
    *out = MK_POSITION(s);
    return RESULT_OK;
}

Result parse_move(const char* s, Move* out) {
    ASSERT_OK(parse_position(s, &out->origin));
    s += sizeof(Position);
    ASSERT_OK(parse_position(s, &out->destination));
    s += sizeof(Position);
    ASSERT_OR(*s == '\0' || strchr("nbrq", *s), INVALID_PROMOTION);
    out->promotion = *s;
    return RESULT_OK;
}
