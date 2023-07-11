#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// No single piece can have more than 32 moves in a turn
#define MAX_MOVES_PIECE 32
#define MAX_NUM_PIECES 16
// Maximum possible number of moves in any turn
#define MAX_MOVES 218
// Maximum number of moves that a game may have
#define HISTORY_MAX 256
// source: https://chess.stackexchange.com/questions/30004/longest-possible-fen
#define MAX_FEN_LENGTH 87
// source: http://page.mi.fu-berlin.de/block/uci.htm
// biggest command is `copyprotection`
#define UCI_MAX_CMD_SIZE 16

static const char* piece_kinds = "pnbrqk";
typedef enum {
    PIECE_PAWN   = 'p',
    PIECE_KNIGHT = 'n',
    PIECE_BISHOP = 'b',
    PIECE_ROOK   = 'r',
    PIECE_QUEEN  = 'q',
    PIECE_KING   = 'k',
} PieceKind;

typedef enum {
    COLOR_WHITE = 0,
    COLOR_BLACK,
} PieceColor;

typedef struct {
    PieceKind kind;
    PieceColor color;
} Piece;

typedef struct {
    bool has_piece;
    Piece piece;
} Square;

typedef struct {
    Square squares[8][8];
} Board;

#define INVALID_POSITION MK_POSITION("iv")
typedef struct {
    char file;
    char rank;
} Position;

#define NO_PROMOTION '\0'
typedef struct {
    Position origin;
    Position destination;
    char promotion; // '\0' if it's not a promotion
} Move;

// TODO: en passant + castle
typedef struct {
    Move move;
    bool is_capture;
    Piece captured;
    int halfmove_clock;
} MoveHistory;

typedef struct {
    PieceColor turn;
    Board board;
    bool has_king_moved[2];
    struct {
        bool kings;
        bool queens;
    } has_rook_moved[2];
    struct {
        bool has;
        Position en_passant;
    } double_pushed;
    int halfmove_clock;
    int fullmove_counter;
} Game;

#define ERROR(name) RESULT_ERR_##name
typedef enum {
    RESULT_OK = 0,
    RESULT_ERR_EOF,
    RESULT_ERR_INVALID_FEN,
    RESULT_ERR_INVALID_UCI,
    RESULT_ERR_INVALID_POSITION,
    RESULT_ERR_INVALID_PROMOTION,
    RESULT_ERR_INVALID_PIECE,
    // Check libc's errno in order to get the error
    RESULT_ERR_LIBC,
} Result;

const char* get_error_msg(Result res);

#define MK_POSITION(s) (*(Position*)s)
#define MK_MOVE(s) (*(Move*)s)

// #define ASSERT(expr) if (!(expr)) return false
#define ASSERT_OK(expr) do {  \
        Result __r = expr;    \
        if (__r != RESULT_OK) \
            return __r;       \
    } while(0)
#define ASSERT_OR(expr, err) if (!(expr)) return RESULT_ERR_##err;
#define STRUCT_EQ(ty, lhs, rhs) (memcmp(lhs, rhs, sizeof(ty)) == 0)

extern const char kings_rank[];

Square* board_index(Position position, Board* board);

void swap_squares(Square* a, Square* b);

PieceColor opposite(PieceColor color);

char piece_letter(Piece piece);

Result piece_from_letter(char letter, Piece* piece);

Result parse_position(const char* s, Position* out);

Result parse_move(const char* s, Move* out);
