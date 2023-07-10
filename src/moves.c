#include <assert.h>
#include <string.h>

#include "common.h"
#include "moves.h"

MoveHistory make_move(Game* game, Move move) {
    Square* square;

    square = board_index(move.origin, &game->board);
    assert(square->has_piece);
    square->has_piece = false;
    Piece piece = square->piece;

    square = board_index(move.destination, &game->board);

    MoveHistory hist;
    hist.move = move;
    hist.is_capture = square->has_piece;
    hist.captured = square->piece;
    hist.halfmove_clock = game->halfmove_clock;

    square->has_piece = true;
    if (move.promotion != '\0') {
        piece.kind = move.promotion;
    }
    square->piece = piece;

    if (hist.is_capture || piece.kind == PIECE_PAWN) {
        game->halfmove_clock = 0;
    } else {
        game->halfmove_clock++;
    }
    // Default is false
    game->double_pushed.has = false;
    if (piece.kind == PIECE_KING) {
        game->has_king_moved[piece.color] = true;
        int offset = move.destination.file - move.origin.file;
        if (abs(offset) == 2) {
            // Must be a castle
            if (offset > 0) {
                // Castle to the king's pawn
                swap_squares(
                    board_index((Position){ .file = 'h', .rank = kings_rank[piece.color] }, &game->board),
                    board_index((Position){ .file = 'f', .rank = kings_rank[piece.color] }, &game->board)
                );
            } else {
                // Castle to the queen's pawn
                swap_squares(
                    board_index((Position){ .file = 'a', .rank = kings_rank[piece.color] }, &game->board),
                    board_index((Position){ .file = 'd', .rank = kings_rank[piece.color] }, &game->board)
                );
            }
        }
    } else if (piece.kind == PIECE_ROOK) {
        Position kings, queens;
        kings.rank = kings_rank[piece.color];
        kings.file = 'h';
        queens.rank = kings_rank[piece.color];
        queens.file = 'a';
        if (!game->has_rook_moved[piece.color].kings && STRUCT_EQ(Position, &move.origin, &kings)) {
            game->has_rook_moved[piece.color].kings = true;
        }
        if (!game->has_rook_moved[piece.color].queens && STRUCT_EQ(Position, &move.origin, &queens)) {
            game->has_rook_moved[piece.color].queens = true;
        }
    } else if (piece.kind == PIECE_PAWN) {
        int offset = move.destination.rank - move.origin.rank;
        if (abs(offset) == 2) {
            // Must be a double push
            game->double_pushed.has = true;
            game->double_pushed.en_passant = move.origin;
            game->double_pushed.en_passant.rank += offset / 2; // +1 square in the direction of the offset
        }
    }

    game->fullmove_counter++;

    return hist;
}

void unmake_move(Game* game, MoveHistory hist) {
    Square* square;

    square = board_index(hist.move.destination, &game->board);
    Piece piece = square->piece;
    if (hist.is_capture) {
        square->piece = hist.captured;
    } else {
        square->has_piece = false;
    }
    square = board_index(hist.move.origin, &game->board);
    square->has_piece = true;
    square->piece = piece;
    game->halfmove_clock = hist.halfmove_clock;
    game->fullmove_counter--;
}

int pawn_moves(Position pawn, PieceColor color, Game* game, Move moves[]) {
    Square* square;
    Move* move = moves;
    Position dest = pawn;
    bool can_take_en_passant;

    switch (color) {
        // White pawns can only go down
        case COLOR_WHITE:
            dest.rank++;
            square = board_index(dest, &game->board);
            if (square && !square->has_piece) {
                move->origin = pawn;
                move->destination = dest;
                move++;
            }
            dest.file++;
            square = board_index(dest, &game->board);
            can_take_en_passant = game->double_pushed.has && STRUCT_EQ(Position, &game->double_pushed.en_passant, &dest);
            if ((square && square->has_piece && square->piece.color != color) || can_take_en_passant) {
                move->origin = pawn;
                move->destination = dest;
                move++;
            }
            dest.file -= 2;
            square = board_index(dest, &game->board);
            can_take_en_passant = game->double_pushed.has && STRUCT_EQ(Position, &game->double_pushed.en_passant, &dest);
            if ((square && square->has_piece && square->piece.color != color) || can_take_en_passant) {
                move->origin = pawn;
                move->destination = dest;
                move++;
            }

            if (pawn.rank == '2') {
                // Double push
                dest = pawn;
                dest.rank += 2;
                if (!board_index(dest, &game->board)->has_piece) {
                    move->origin = pawn;
                    move->destination = dest;
                    move++;
                }
            }
            break;

        // Black pawns can only go up
        case COLOR_BLACK:
            dest.rank--;
            square = board_index(dest, &game->board);
            if (square && !square->has_piece) {
                move->origin = pawn;
                move->destination = dest;
                move++;
            }
            dest.file++;
            square = board_index(dest, &game->board);
            can_take_en_passant = game->double_pushed.has && STRUCT_EQ(Position, &game->double_pushed.en_passant, &dest);
            if ((square && square->has_piece && square->piece.color != color) || can_take_en_passant) {
                move->origin = pawn;
                move->destination = dest;
                move++;
            }
            dest.file -= 2;
            square = board_index(dest, &game->board);
            can_take_en_passant = game->double_pushed.has && STRUCT_EQ(Position, &game->double_pushed.en_passant, &dest);
            if ((square && square->has_piece && square->piece.color != color) || can_take_en_passant) {
                move->origin = pawn;
                move->destination = dest;
                move++;
            }

            if (pawn.rank == '7') {
                // Double push
                dest = pawn;
                dest.rank -= 2;
                if (!board_index(dest, &game->board)->has_piece) {
                    move->origin = pawn;
                    move->destination = dest;
                    move++;
                }
            }
            break;
    }

    Move* end_without_promotions = move;
    // Add promotions
    for (Move* m = moves; m < end_without_promotions; m++) {
        if (m->destination.rank == kings_rank[opposite(color)]) {
            // We skip the pawn piece, since we can't promote to it.
            m->promotion = PIECE_KING;
            for (const char* promotion = piece_kinds + 2; *promotion != 'k'; promotion++) {
                move->origin = pawn;
                move->destination = m->destination;
                move->promotion = *promotion;
                move++;
            }
        }
    }

    return move - moves;
}

int knight_moves(Position knight, PieceColor color, Board* board, Move moves[]) {
    Position dest;
    Square* square;
    Move* move = moves;

    // Down
    dest = knight;
    dest.rank += 2;
    dest.file++;
    square = board_index(dest, board);
    if (square && (!square->has_piece || square->piece.color != color)) {
        move->origin = knight;
        move->destination = dest;
        move++;
    }
    dest.file -= 2;
    square = board_index(dest, board);
    if (square && (!square->has_piece || square->piece.color != color)) {
        move->origin = knight;
        move->destination = dest;
        move++;
    }

    // Up
    dest = knight;
    dest.rank -= 2;
    dest.file++;
    square = board_index(dest, board);
    if (square && (!square->has_piece || square->piece.color != color)) {
        move->origin = knight;
        move->destination = dest;
        move++;
    }
    dest.file -= 2;
    square = board_index(dest, board);
    if (square && (!square->has_piece || square->piece.color != color)) {
        move->origin = knight;
        move->destination = dest;
        move++;
    }

    // Right
    dest = knight;
    dest.file += 2;
    dest.rank++;
    square = board_index(dest, board);
    if (square && (!square->has_piece || square->piece.color != color)) {
        move->origin = knight;
        move->destination = dest;
        move++;
    }
    dest.rank -= 2;
    square = board_index(dest, board);
    if (square && (!square->has_piece || square->piece.color != color)) {
        move->origin = knight;
        move->destination = dest;
        move++;
    }

    // Left
    dest = knight;
    dest.file -= 2;
    dest.rank++;
    square = board_index(dest, board);
    if (square && (!square->has_piece || square->piece.color != color)) {
        move->origin = knight;
        move->destination = dest;
        move++;
    }
    dest.rank -= 2;
    square = board_index(dest, board);
    if (square && (!square->has_piece || square->piece.color != color)) {
        move->origin = knight;
        move->destination = dest;
        move++;
    }

    return move - moves;
}

int bishop_moves(Position bishop, PieceColor color, Board* board, Move moves[]) {
    Position dest;
    Square* square;
    Move* move = moves;

    // Down-Right
    for (dest = bishop; ++dest.rank <= '8' && ++dest.file <= 'h' && !board_index(dest, board)->has_piece; move++) {
        move->origin = bishop;
        move->destination = dest;
    }
    square = board_index(dest, board);
    if (square && square->piece.color != color) {
        move->origin = bishop;
        move->destination = dest;
        move++;
    }

    // Down-Left
    for (dest = bishop; ++dest.rank <= '8' && --dest.file >= 'a' && !board_index(dest, board)->has_piece; move++) {
        move->origin = bishop;
        move->destination = dest;
    }
    square = board_index(dest, board);
    if (square && square->piece.color != color) {
        move->origin = bishop;
        move->destination = dest;
        move++;
    }

    // Up-Right
    for (dest = bishop; --dest.rank >= '1' && ++dest.file <= 'h' && !board_index(dest, board)->has_piece; move++) {
        move->origin = bishop;
        move->destination = dest;
    }
    square = board_index(dest, board);
    if (square && square->piece.color != color) {
        move->origin = bishop;
        move->destination = dest;
        move++;
    }

    // Up-Left
    for (dest = bishop; --dest.rank >= '1' && --dest.file >= 'a' && !board_index(dest, board)->has_piece; move++) {
        move->origin = bishop;
        move->destination = dest;
    }
    square = board_index(dest, board);
    if (square && square->piece.color != color) {
        move->origin = bishop;
        move->destination = dest;
        move++;
    }

    return move - moves;
}

int rook_moves(Position rook, PieceColor color, Board* board, Move moves[]) {
    Position dest;
    Square* square;
    Move* move = moves;

    // Down
    for (dest = rook; ++dest.rank <= '8' && !board_index(dest, board)->has_piece; move++) {
        move->origin = rook;
        move->destination = dest;
    }
    square = board_index(dest, board);
    if (square && square->piece.color != color) {
        move->origin = rook;
        move->destination = dest;
        move++;
    }

    // Up
    for (dest = rook; --dest.rank >= '1' && !board_index(dest, board)->has_piece; move++) {
        move->origin = rook;
        move->destination = dest;
    }
    square = board_index(dest, board);
    if (square && square->piece.color != color) {
        move->origin = rook;
        move->destination = dest;
        move++;
    }

    // Right
    for (dest = rook; ++dest.file <= 'h' && !board_index(dest, board)->has_piece; move++) {
        move->origin = rook;
        move->destination = dest;
    }
    square = board_index(dest, board);
    if (square && square->piece.color != color) {
        move->origin = rook;
        move->destination = dest;
        move++;
    }

    // Left
    for (dest = rook; --dest.file >= 'a' && !board_index(dest, board)->has_piece; move++) {
        move->origin = rook;
        move->destination = dest;
    }
    square = board_index(dest, board);
    if (square && square->piece.color != color) {
        move->origin = rook;
        move->destination = dest;
        move++;
    }

    return move - moves;
}

int queen_moves(Position queen, PieceColor color, Board* board, Move moves[]) {
    Move* move = moves;
    move += bishop_moves(queen, color, board, move);
    move += rook_moves(queen, color, board, move);
    return move - moves;
}

int king_moves(Position king, PieceColor color, Game* game, Move moves[]) {
    Move* move = moves;
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            Position dest = king;
            dest.rank += dr;
            dest.file += dc;
            Square* square = board_index(dest, &game->board);
            if (square && (!square->has_piece || square->piece.color != color)) {
                move->origin = king;
                move->destination = dest;
            }
        }
    }
    if (!game->has_king_moved[color]) {
        if (!game->has_rook_moved[color].kings) {
            bool is_path_clear = false;
            Position pos;
            pos.rank = kings_rank[color];
            for (pos.file = 'f'; pos.file < 'h'; pos.file++) {
                if (board_index(pos, &game->board)->has_piece) {
                    is_path_clear = false;
                    break;
                }
            }
            if (is_path_clear) {
                pos.file = 'g';
                move->origin = king;
                move->destination = pos;
                move++;
            }
        }
        if (!game->has_rook_moved[color].queens) {
            bool is_path_clear = false;
            Position pos;
            pos.rank = kings_rank[color];
            for (pos.file = 'b'; pos.file < 'e'; pos.file++) {
                if (board_index(pos, &game->board)->has_piece) {
                    is_path_clear = false;
                    break;
                }
            }
            if (is_path_clear) {
                pos.file = 'c';
                move->origin = king;
                move->destination = pos;
                move++;
            }
        }
    }
    return move - moves;
}

int piece_moves(Position position, Piece piece, Game* game, Move moves[]) {
    switch (piece.kind) {
        case PIECE_PAWN:
            return pawn_moves(position, piece.color, game, moves);
        case PIECE_KNIGHT:
            return knight_moves(position, piece.color, &game->board, moves);
        case PIECE_BISHOP:
            return bishop_moves(position, piece.color, &game->board, moves);
        case PIECE_ROOK:
            return rook_moves(position, piece.color, &game->board, moves);
        case PIECE_QUEEN:
            return queen_moves(position, piece.color, &game->board, moves);
        case PIECE_KING:
            return king_moves(position, piece.color, game, moves);
    }
}

bool is_position_attacked_by_moves(Position pos, Move moves[], int nmoves) {
    for (int i = 0; i < nmoves; i++) {
        if (STRUCT_EQ(Position, &moves[i].destination, &pos)) {
            return true;
        }
    }
    return false;
}

int remove_invalid_moves(Piece piece, Game* game, Move moves[], int nmoves) {
    Position enemy_pieces[16];
    Position* enemy_piece_top = enemy_pieces;

    Position ally_king;
    Position pos;
    for (pos.rank = '1'; pos.rank <= '8'; pos.rank++) {
        for (pos.file = 'a'; pos.file <= 'h'; pos.file++) {
            Square* square = board_index(pos, &game->board);
            if (square->has_piece) {
                if (square->piece.color != piece.color) {
                    *(enemy_piece_top++) = pos;
                } else if (square->piece.kind == PIECE_KING) {
                    ally_king = pos;
                }
            }
        }
    }


    for (int i = nmoves-1; i >= 0; i--) {
        Move move = moves[i];
        Game clone;
        memcpy(&clone, game, sizeof(Game));
        make_move(&clone, move);

        Move enemy_moves[MAX_MOVES];
        memset(enemy_moves, 0, sizeof(enemy_moves));
        Move* enemy_moves_top = enemy_moves;
        for (Position* enemy_piece = enemy_pieces; enemy_piece < enemy_piece_top; enemy_piece++) {
            // If the current move takes an enemy piece, we simply skip that enemy.
            if (STRUCT_EQ(Position, enemy_piece, &move.destination)) continue;
            Piece piece = board_index(*enemy_piece, &clone.board)->piece;
            enemy_moves_top += piece_moves(*enemy_piece, piece, &clone, enemy_moves_top);
        }
        int n_enemy_moves = enemy_moves_top - enemy_moves;
        if (is_position_attacked_by_moves(ally_king, enemy_moves, n_enemy_moves)) {
            // King in check after move, the move is invalid
            moves[i] = moves[--nmoves]; // swap remove
        }
    }
    return nmoves;
}

int valid_piece_moves(Position position, Piece piece, Game* game, Move moves[]) {
    int count = piece_moves(position, piece, game, moves);
    return remove_invalid_moves(piece, game, moves, count);
}

int all_valid_moves(Game* game, Move moves[]) {
    Move* moves_top = moves;
    Position pos;
    for (pos.rank = '1'; pos.rank <= '8'; pos.rank++) {
        for (pos.file = 'a'; pos.file <= 'h'; pos.file++) {
            Square* square = board_index(pos, &game->board);
            if (square->has_piece && square->piece.color == game->turn)
                moves_top += valid_piece_moves(pos, square->piece, game, moves_top);
        }
    }
    return moves_top - moves;
}

bool check_move(Move move, Game* game, Move valid_moves[], int nmoves) {
    for (int i = 0; i < nmoves; i++) {
        if (STRUCT_EQ(Move, valid_moves + i, &move))
            return true;
    }
    return false;
}

bool is_move_valid(Move move, Game* game) {
    Square* square = board_index(move.origin, &game->board);
    if (!square || !square->has_piece || square->piece.color != game->turn) return false;

    Move moves[MAX_MOVES_PIECE];
    memset(moves, 0, sizeof(moves));
    int count = valid_piece_moves(move.origin, square->piece, game, moves);
    return check_move(move, game, moves, count);
}
