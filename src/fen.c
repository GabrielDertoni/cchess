// source: https://www.chessprogramming.org/Forsyth-Edwards_Notation
//
//     <FEN> ::=  <Piece Placement>
//            ' ' <Side to move>
//            ' ' <Castling ability>
//            ' ' <En passant target square>
//            ' ' <Halfmove clock>
//            ' ' <Fullmove counter>
//
//    <Piece Placement>          ::= <rank8>'/'<rank7>'/'<rank6>'/'<rank5>'/'<rank4>'/'<rank3>'/'<rank2>'/'<rank1>
//    <ranki>                    ::= [<digit17>]<piece> {[<digit17>]<piece>} [<digit17>] | '8'
//    <piece>                    ::= <white Piece> | <black Piece>
//    <digit17>                  ::= '1' | '2' | '3' | '4' | '5' | '6' | '7'
//    <white Piece>              ::= 'P' | 'N' | 'B' | 'R' | 'Q' | 'K'
//    <black Piece>              ::= 'p' | 'n' | 'b' | 'r' | 'q' | 'k'
//
//    <Side to move>             ::= {'w' | 'b'}
//    <Castling ability>         ::= '-' | ['K'] ['Q'] ['k'] ['q'] (1..4)
//    <En passant target square> ::= '-' | <epsquare>
//    <epsquare>                 ::= <fileLetter> <eprank>
//    <fileLetter>               ::= 'a' | 'b' | 'c' | 'd' | 'e' | 'f' | 'g' | 'h'
//    <eprank>                   ::= '3' | '6'
//
//    <Halfmove Clock>           ::= <digit> {<digit>}
//    <digit>                    ::= '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'
//
//    <Fullmove counter>         ::= <digit19> {<digit>}
//    <digit19>                  ::= '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'
//    <digit>                    ::= '0' | <digit19>
//

#include "common.h"

const char* FEN_STARTING = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

void game_fen(Game* game, char* out) {
    // Piece placement
    Position pos;
    for (pos.rank = '8'; pos.rank >= '1'; pos.rank--) {
        int consecutive_spaces = 0;
        for (pos.file = 'a'; pos.file <= 'h'; pos.file++) {
            Square* square = board_index(pos, &game->board);
            if (square->has_piece) {
                if (consecutive_spaces != 0) {
                    *out++ = consecutive_spaces + '0';
                    consecutive_spaces = 0;
                }
                *out++ = piece_letter(square->piece);
            } else {
                consecutive_spaces++;
            }
        }
        if (consecutive_spaces != 0) {
            *out++ = consecutive_spaces + '0';
        }
        if (pos.rank > '1') {
            *out++ = '/';
        }
    }
    *out++ = ' ';

    // Side to move
    *out++ = game->turn == COLOR_WHITE ? 'w' : 'b';
    *out++ = ' ';

    // Castling ability
    char *save = out;
    for (PieceColor color = COLOR_WHITE; color <= COLOR_BLACK; color++) {
        if (!game->has_king_moved[color]) {
            if (!game->has_rook_moved[color].kings)
                *out++ = piece_letter((Piece) { .color = color, .kind = PIECE_KING });
            if (!game->has_rook_moved[color].queens)
                *out++ = piece_letter((Piece) { .color = color, .kind = PIECE_QUEEN });
        }
    }
    // If we wrote nothing for castling, write '-'
    if (out == save)
        *out++ = '-';
    *out++ = ' ';

    // En passant
    if (game->double_pushed.has) {
        *out++ = game->double_pushed.en_passant.file;
        *out++ = game->double_pushed.en_passant.rank;
    } else {
        *out++ = '-';
    }
    *out++ = ' ';

    // Halfmove clock
    out += sprintf(out, "%d ", game->halfmove_clock);

    // Fullmove counter
    out += sprintf(out, "%d", game->fullmove_counter);

    *out = '\0';
}

Result parse_fen(Game* this, const char* fen) {
    if (!fen) fen = FEN_STARTING;

    Position pos;
    for (pos.rank = '8'; pos.rank >= '1'; pos.rank--) {
        int consecutive_spaces = 0;
        for (pos.file = 'a'; pos.file <= 'h'; pos.file++) {
            Square* square = board_index(pos, &this->board);
            if (consecutive_spaces != 0 || (*fen >= '0' && *fen <= '9')) {
                if (consecutive_spaces == 0) {
                    consecutive_spaces = *fen++ - '0';
                }
                square->has_piece = false;
                consecutive_spaces--;
            } else {
                square->has_piece = true;
                ASSERT_OK(piece_from_letter(*fen++, &square->piece));
            }
        }
        ASSERT_OR(pos.rank == '1' || *fen++ == '/', INVALID_FEN);
    }
    ASSERT_OR(*fen++ == ' ', INVALID_FEN);

    char side_to_move = *(fen++);
    ASSERT_OR(side_to_move == 'w' || side_to_move == 'b', INVALID_FEN);
    this->turn = side_to_move == 'w' ? COLOR_WHITE : COLOR_BLACK;
    ASSERT_OR(*fen++ == ' ', INVALID_FEN);

    if (*fen != '-') {
        while (*fen != ' ') {
            Piece piece;
            ASSERT_OK(piece_from_letter(*fen++, &piece));
            this->has_king_moved[piece.color] = false;
            if (piece.kind == PIECE_KING)
                this->has_rook_moved[piece.color].kings = true;
            else if (piece.kind == PIECE_QUEEN)
                this->has_rook_moved[piece.color].queens = true;
            else
                return false;
        }
    } else {
        fen++;
    }
    ASSERT_OR(*fen++ == ' ', INVALID_FEN);

    if (*fen != '-') {
        this->double_pushed.has = true;
        ASSERT_OK(parse_position(fen, &this->double_pushed.en_passant));
        fen += 2;
    } else {
        fen++;
    }
    ASSERT_OR(*fen++ == ' ', INVALID_FEN);

    int count;
    ASSERT_OR(sscanf(fen, "%d%n", &this->halfmove_clock, &count) != EOF, INVALID_FEN);
    fen += count;
    ASSERT_OR(*fen++ == ' ', INVALID_FEN);

    ASSERT_OR(sscanf(fen, "%d%n", &this->fullmove_counter, &count) != EOF, INVALID_FEN);
    fen += count;
    ASSERT_OR(*fen == '\0', INVALID_FEN);

    return RESULT_OK;
}
