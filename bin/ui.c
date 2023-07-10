#include <stdarg.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/in.h>

#include "common.h"
#include "moves.h"
#include "fen.h"
#include "uci.h"

#define MAX_PATH 128
#define PORT 8080

typedef struct {
    Game game;
    PieceColor color;
    bool has_selected;
    Position selected;
    size_t n_moves;
    bool waiting_move;
    Move moves[MAX_MOVES];
} GameUI;

static char path[MAX_PATH];
static FILE* in = NULL;
static FILE* out = NULL;
static char* line = NULL;
static size_t linecap = 0;
static GameUI ui;
static int server_fd = -1;

char const* const light_square = "#f8ce9e";
char const* const dark_square  = "#d18b47";

int piece_svg(FILE* out, Piece piece) {
    extern unsigned char WHITE_PAWN_SVG[];
    extern unsigned int  WHITE_PAWN_SVG_LEN;
    extern unsigned char WHITE_KNIGHT_SVG[];
    extern unsigned int  WHITE_KNIGHT_SVG_LEN;
    extern unsigned char WHITE_BISHOP_SVG[];
    extern unsigned int  WHITE_BISHOP_SVG_LEN;
    extern unsigned char WHITE_ROOK_SVG[];
    extern unsigned int  WHITE_ROOK_SVG_LEN;
    extern unsigned char WHITE_QUEEN_SVG[];
    extern unsigned int  WHITE_QUEEN_SVG_LEN;
    extern unsigned char WHITE_KING_SVG[];
    extern unsigned int  WHITE_KING_SVG_LEN;

    extern unsigned char BLACK_PAWN_SVG[];
    extern unsigned int  BLACK_PAWN_SVG_LEN;
    extern unsigned char BLACK_KNIGHT_SVG[];
    extern unsigned int  BLACK_KNIGHT_SVG_LEN;
    extern unsigned char BLACK_BISHOP_SVG[];
    extern unsigned int  BLACK_BISHOP_SVG_LEN;
    extern unsigned char BLACK_ROOK_SVG[];
    extern unsigned int  BLACK_ROOK_SVG_LEN;
    extern unsigned char BLACK_QUEEN_SVG[];
    extern unsigned int  BLACK_QUEEN_SVG_LEN;
    extern unsigned char BLACK_KING_SVG[];
    extern unsigned int  BLACK_KING_SVG_LEN;

    typedef struct {
        const unsigned char* content;
        unsigned int* len;
    } Svg;

    static Svg piece_svg[][2] = {
        [PIECE_PAWN] = {
            [COLOR_WHITE] = { .content = WHITE_PAWN_SVG, .len = &WHITE_PAWN_SVG_LEN },
            [COLOR_BLACK] = { .content = BLACK_PAWN_SVG, .len = &BLACK_PAWN_SVG_LEN },
        },
        [PIECE_KNIGHT] = {
            [COLOR_WHITE] = { .content = WHITE_KNIGHT_SVG, .len = &WHITE_KNIGHT_SVG_LEN },
            [COLOR_BLACK] = { .content = BLACK_KNIGHT_SVG, .len = &BLACK_KNIGHT_SVG_LEN },
        },
        [PIECE_BISHOP] = {
            [COLOR_WHITE] = { .content = WHITE_BISHOP_SVG, .len = &WHITE_BISHOP_SVG_LEN },
            [COLOR_BLACK] = { .content = BLACK_BISHOP_SVG, .len = &BLACK_BISHOP_SVG_LEN },
        },
        [PIECE_ROOK] = {
            [COLOR_WHITE] = { .content = WHITE_ROOK_SVG, .len = &WHITE_ROOK_SVG_LEN },
            [COLOR_BLACK] = { .content = BLACK_ROOK_SVG, .len = &BLACK_ROOK_SVG_LEN },
        },
        [PIECE_QUEEN] = {
            [COLOR_WHITE] = { .content = WHITE_QUEEN_SVG, .len = &WHITE_QUEEN_SVG_LEN },
            [COLOR_BLACK] = { .content = BLACK_QUEEN_SVG, .len = &BLACK_QUEEN_SVG_LEN },
        },
        [PIECE_KING] = {
            [COLOR_WHITE] = { .content = WHITE_KING_SVG, .len = &WHITE_KING_SVG_LEN },
            [COLOR_BLACK] = { .content = BLACK_KING_SVG, .len = &BLACK_KING_SVG_LEN },
        },
    };

    Svg svg = piece_svg[piece.kind][piece.color];
    return fwrite(svg.content, sizeof(unsigned char), *svg.len, out);
}

int render_chessboard(FILE* out) {
    Game* game = &ui.game;
    int width = 400;
    int height = 400;

    int count = 0;
    int res;

    // res = fprintf(out, "<?xml version=\"1.0\" standalone=\"no\"?>\n");
    // if (res < 0) return res;
    // count += res;

    res = fprintf(out,
        "<svg id=\"chessboard\" width=\"%d\" height=\"%d\" viewBox=\"0 0 8 8\" xmlns=\"http://www.w3.org/2000/svg\">\n",
        width, height);
    if (res < 0) return res;
    count += res;

    Position pos;
    for (pos.rank = '1'; pos.rank <= '8'; pos.rank++) {
        for (pos.file = 'a'; pos.file <= 'h'; pos.file++) {
            int x = pos.file - 'a';
            int y = pos.rank - '1';
            const char* fill = (x + y % 2) % 2 == 0 ? light_square : dark_square;
            res = fprintf(out,
                "<rect "
                    "hx-get=\"/click\" "
                    "hx-vals='{\"square\": \"%c%c\"}' "
                    "hx-trigger=\"click\" "
                    "hx-target=\"#chessboard\" "
                    "hx-swap=\"outerHTML\" "
                    "x=\"%d\" "
                    "y=\"%d\" "
                    "width=\"1\" "
                    "height=\"1\" "
                    "fill=\"%s\" "
                    "stroke=\"none\""
                "/>\n",
                pos.file, pos.rank, x, y, fill);
            if (res < 0) return res;
            count += res;

            if (ui.has_selected && STRUCT_EQ(Position, &pos, &ui.selected)) {
                const char* highlight = "#ffff00";
                res = fprintf(out,
                    "<rect "
                        "x=\"%d\" "
                        "y=\"%d\" "
                        "width=\"1\" "
                        "height=\"1\" "
                        "fill=\"%s\" "
                        "fill-opacity=\"0.5\" "
                        "stroke=\"none\""
                    "/>",
                    x, y, highlight
                );
                if (res < 0) return res;
                count += res;
            }

            Square* square = board_index(pos, &game->board);
            if (square->has_piece) {
                res = fprintf(out, "<g transform=\"translate(%d, %d)\" style=\"pointer-events: none\">\n", x, y);
                if (res < 0) return res;
                count += res;

                res = piece_svg(out, square->piece);
                if (res < 0) return res;
                count += res;

                res = fprintf(out, "</g>\n");
                if (res < 0) return res;
                count += res;
            }
        }
    }

    if (ui.has_selected) {
        for (int i = 0; i < ui.n_moves; i++) {
            Position dest = ui.moves[i].destination;
            int x = dest.file - 'a';
            int y = dest.rank - '1';
            res = fprintf(out,
                "<circle "
                    "cx=\"%d.5\" "
                    "cy=\"%d.5\" "
                    "r=\"0.2\" "
                    "fill=\"#000000\" "
                    "fill-opacity=\"0.1\" "
                    "stroke=\"none\""
                    "style=\"pointer-events: none\""
                "/>",
                x, y
            );
            if (res < 0) return res;
            count += res;
        }
    }

    res = fprintf(out, "</svg>\n");
    if (res < 0) return res;
    count += res;

    return count;
}

int render_html(FILE* out) {
    Game* game = &ui.game;
    int count = 0;
    int res;

    res = fprintf(out, "<!DOCTYPE html>\n"
                       "<html>\n"
                       "<head>\n"
                       "    <title>chess</title>\n"
                       "    <script src=\"https://unpkg.com/htmx.org@1.9.2\" integrity=\"sha384-L6OqL9pRWyyFU3+/bjdSri+iIphTN/bvYyM37tICVyOJkWZLpP2vGn6VUEXgzg6h\" crossorigin=\"anonymous\"></script>\n"
                       "</head>\n"
                       "<body>\n");
    if (res < 0) return res;
    count += res;

    res = render_chessboard(out);
    if (res < 0) return res;
    count += res;

    res = fprintf(out, "</body>\n</html>\n");
    if (res < 0) return res;
    count += res;

    return res;
}

#define ASSERT_LIBC(expr, msg) do { \
        if (!(expr)) {              \
            perror(msg);            \
            exit(EXIT_FAILURE);     \
        }                           \
    } while (0)

#define HTTP_MAX_HEADERS 64
#define HTTP_MAX_REQUEST_PATH 128
#define HTTP_MAX_PROTO 16
#define HTTP_MAX_STATUS_MSG 16

typedef enum {
    HTTP_GET = 0,
    HTTP_HEAD,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_CONNECT,
    HTTP_OPTIONS,
    HTTP_PATCH,

    HTTP_COUNT,
} HttpMethod;

typedef struct {
    char* key;
    char* value;
} HttpHeader;

typedef struct {
    HttpMethod method;
    char path[HTTP_MAX_REQUEST_PATH];
    char proto[HTTP_MAX_PROTO];
    // null-terminated
    HttpHeader headers[HTTP_MAX_HEADERS];
    char body[];
} HttpRequest;

typedef struct {
    char proto[HTTP_MAX_PROTO];
    int status;
    char status_msg[HTTP_MAX_STATUS_MSG];
    HttpHeader headers[HTTP_MAX_HEADERS];
    char body[];
} HttpResponse;

#define METHOD(name) [HTTP_##name] = #name

static const char* HTTP_METHOD_NAME[] = {
    METHOD(GET),
    METHOD(HEAD),
    METHOD(POST),
    METHOD(PUT),
    METHOD(DELETE),
    METHOD(CONNECT),
    METHOD(OPTIONS),
    METHOD(PATCH),
};

void eout(const char* fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    fprintf(stderr, "[error] ");
    vfprintf(stderr, fmt, argp);
    fprintf(stderr, "\n");
    va_end(argp);
}

void write_response(FILE* conn, HttpResponse* response, size_t content_length) {
    fprintf(conn, "%s %d %s\r\n", response->proto, response->status, response->status_msg);
    for (HttpHeader* header = response->headers; header->key; header++) {
        fprintf(conn, "%s: %s\r\n", header->key, header->value);
    }
    fprintf(conn, "\r\n");
    if (content_length > 0) {
        fwrite(response->body, 1, content_length, conn);
        fflush(conn);
    }
}

void bad_request(FILE* conn) {
    HttpResponse response;
    strcpy(response.proto, "HTTP/1.0");
    response.status = 400;
    strcpy(response.status_msg, "Bad Request");
    memset(response.headers, 0, sizeof(response.headers));
    write_response(conn, &response, 0);
}

HttpHeader* find_header(HttpRequest* msg, const char* key) {
    for (HttpHeader* ptr = msg->headers; ptr->key; ptr++) {
        if (strcmp(ptr->key, key) == 0)
            return ptr;
    }
    return NULL;
}

void request_deinit(HttpRequest* request) {
    for (int i = 0; request->headers[i].key; i++) {
        free(request->headers[i].key);
        free(request->headers[i].value);
    }
    free(request);
}

HttpRequest* read_request(FILE* conn) {
    HttpRequest* request = malloc(sizeof(HttpRequest));
    if (!request) goto fail;
    memset(request->headers, 0, sizeof(request->headers));

    if (getline(&line, &linecap, conn) == EOF) goto fail;

    char* parse = line;
    char* method_str = strsep(&parse, " ");
    HttpMethod method;
    for (method = 0; method < HTTP_COUNT && strcmp(HTTP_METHOD_NAME[method], method_str) != 0; method++);
    if (method == HTTP_COUNT) {
        eout("invalid http method");
        goto fail;
    }
    request->method = method;

    if (!parse) {
        eout("no path");
        goto fail;
    }
    char* path = strsep(&parse, " ");
    if (strlen(path) >= HTTP_MAX_REQUEST_PATH) {
        eout("path too long");
        goto fail;
    }
    strcpy(request->path, path);
    if (!parse) {
        eout("no protocol");
        goto fail;
    }
    char* version = strdup(strsep(&parse, " "));
    if (strlen(version) >= HTTP_MAX_PROTO) {
        eout("protocol version too long");
        goto fail;
    }
    strcpy(request->proto, version);
    char* version_protocol = strsep(&version, "/");
    char* version_number = version;

    if (!version_protocol || strcmp(version_protocol, "HTTP") != 0) {
        eout("expected protocol to be 'HTTP', got '%s' instead", version_protocol);
        goto fail;
    }
    if (!version_number) {
        eout("no version number");
        goto fail;
    }

    for (int i = 0; i < HTTP_MAX_HEADERS; i++) {
        if (getline(&line, &linecap, conn) == EOF) goto fail;
        if (line[0] == '\r' || line[0] == '\n') break;
        parse = line;
        request->headers[i].key = strdup(strsep(&parse, ":"));
        if (!parse) {
            eout("no value in request header");
            goto fail;
        }
        request->headers[i].value = strdup(strsep(&parse, "\r\n"));
    }
    if (line[0] != '\r' && line[0] != '\n') {
        eout("header limit exceeded");
        goto fail;
    }

    HttpHeader* content_length = find_header(request, "Content-Length");
    if (content_length) {
        char* endp;
        long len = strtol(content_length->value, &endp, 10);
        if (content_length->value == endp) {
            eout("invalid 'Content-Length' header");
            goto fail;
        }
        request = realloc(request, sizeof(HttpRequest) + len);
        if (fread(request->body, 1, len, conn) == -1)
            goto fail;
    }

    return request;
fail:
    if (request) request_deinit(request);
    return NULL;
}

char* get_date() {
    FILE* fp = popen("date -u", "r");
    if (!fp) return NULL;
    if (getline(&line, &linecap, fp) == -1) {
        pclose(fp);
        return NULL;
    }
    // Trim newline character
    char* parse = line;
    char* ret = strdup(strsep(&parse, "\r\n"));
    pclose(fp);
    return ret;
}

void handle_request(FILE* conn) {
    errno = 0;
    HttpRequest* request = read_request(conn);
    if (!request) {
        if (errno != 0)
            perror("read_request");
        else
            bad_request(conn);
        goto teardown;
    }

    char path[HTTP_MAX_REQUEST_PATH];
    strcpy(path, request->path);

    char* parse = path;
    char* command = strsep(&parse, "?");
    char* date = get_date();
    if (!date) {
        eout("failed to get date");
        goto teardown;
    }

    if (request->method != HTTP_GET) {
        eout("unexpected http method %s", HTTP_METHOD_NAME[request->method]);
        goto teardown;
    }

    fprintf(stderr, "GET %s\n", command);

    if (strcmp(command, "/click") == 0) {
        char* square_str = strsep(&parse, "=");
        if (!square_str || strcmp(square_str, "square") != 0) {
            eout("expected 'square' as first query param, got '%s'", square_str);
            bad_request(conn);
            goto teardown;
        }
        Position pos;
        if (parse_position(parse, &pos) != RESULT_OK) {
            eout("invalid selected square '%.2s'", (char*)&pos);
            bad_request(conn);
            goto teardown;
        }
        Square* square = board_index(pos, &ui.game.board);
        if (square->has_piece && square->piece.color == ui.color) {
            ui.has_selected = true;
            ui.selected = pos;
            ui.n_moves = valid_piece_moves(pos, square->piece, &ui.game, ui.moves);
        } else {
            if (ui.has_selected) {
                int i;
                for (i = 0; i < ui.n_moves && !STRUCT_EQ(Position, &ui.moves[i].destination, &pos); i++);
                if (i < ui.n_moves) {
                    Move move = {
                        .origin = ui.selected,
                        .destination = pos,
                        .promotion = NO_PROMOTION,
                    };
                    if (ui.game.turn == ui.color && ui.waiting_move) {
                        make_move(&ui.game, move);
                        fprintf(out, "bestmove %.5s\n", (char*)&move);
                        ui.waiting_move = false;
                    }
                }
            }
            ui.has_selected = false;
        }

        HttpResponse response = {
            .proto = "HTTP/1.0",
            .status = 200,
            .status_msg = "OK",
            .headers = {
                { .key = "Date", .value = date },
                { .key = "Content-Type", .value = "text/html" },
                { 0 },
            },
        };
        write_response(conn, &response, 0);
        render_chessboard(conn);
    } else if (strcmp(command, "/") == 0) {
        HttpResponse response = {
            .proto = "HTTP/1.0",
            .status = 200,
            .status_msg = "OK",
            .headers = {
                { .key = "Date", .value = date },
                { .key = "Content-Type", .value = "text/html" },
                { 0 },
            },
        };
        write_response(conn, &response, 0);
        render_html(conn);
        fflush(conn);
    }

teardown:
    if (date) free(date);
    if (request) request_deinit(request);
}

void usage_exit(const char* progname) {
    fprintf(stderr, "Usage: %s COLOR PATH\n", progname);
    exit(EXIT_FAILURE);
}

void parse_args(int argc, char* const argv[]) {
    if (argc < 3) usage_exit(argv[0]);
    if (strcmp(argv[1], "white") == 0) {
        ui.color = COLOR_WHITE;
    } else if (strcmp(argv[1], "black") == 0) {
        ui.color = COLOR_BLACK;
    } else {
        eout("COLOR argument must be either 'black' or 'white'");
        usage_exit(argv[0]);
    }
    if (strlen(argv[2]) >= MAX_PATH) {
        eout("maximum path name length exceeded");
        usage_exit(argv[0]);
    }
    strcpy(path, argv[2]);
}

void uci_handshake() {
    char* parse;
    char* uci_cmd;

    fprintf(stderr, "waiting for UCI to connect\n");
    ASSERT_LIBC(getline(&line, &linecap, in) != EOF, "getline");
    parse = line;
    uci_cmd = strsep(&parse, "\r\n");
    if (strcmp(uci_cmd, "uci") != 0) {
        eout("expected 'uci' command");
        exit(EXIT_FAILURE);
    }
    fprintf(out, "uciok\n");

    ASSERT_LIBC(getline(&line, &linecap, in) != EOF, "getline");
    parse = line;
    uci_cmd = strsep(&parse, "\r\n");
    if (strcmp(uci_cmd, "isready") != 0) {
        eout("expected 'isready' command");
        exit(EXIT_FAILURE);
    }
    fprintf(out, "readyok\n");
    fprintf(stderr, "UCI connected\n");
}

void init() {
    char pathbuf[MAX_PATH + 16];

    sprintf(pathbuf, "%s/out", path);
    ASSERT_LIBC(in = fopen(pathbuf, "r+"), "fopen");
    setbuf(in, NULL);

    sprintf(pathbuf, "%s/in", path);
    ASSERT_LIBC(out = fopen(pathbuf, "w+"), "fopen");
    setlinebuf(out);

    uci_handshake();

    parse_fen(&ui.game, FEN_STARTING);
    ui.has_selected = false;
    ui.waiting_move = false;

    ASSERT_LIBC((server_fd = socket(AF_INET, SOCK_STREAM, 0)) != -1, "socket");
    int option = 1;
    ASSERT_LIBC(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) != -1, "setsockopt");

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    ASSERT_LIBC(bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) != -1, "bind");
    ASSERT_LIBC(listen(server_fd, 1) != -1, "listen");

    fprintf(stderr, "[info] listening on port %d\n", PORT);
}

int main(int argc, char* const argv[]) {
    parse_args(argc, argv);
    init();

    struct sockaddr_in peer_addr;
    socklen_t peer_addr_size = sizeof(peer_addr);
    int peerfd;

    for (;;) {
        int nfds = 0;
        fd_set readfs;
        FD_ZERO(&readfs);

        FD_SET(server_fd, &readfs);
        if (server_fd > nfds) nfds = server_fd;
        FD_SET(fileno(in), &readfs);
        if (fileno(in) > nfds) nfds = fileno(in);

        ASSERT_LIBC(select(nfds + 1, &readfs, NULL, NULL, NULL) != -1, "select");

        if (FD_ISSET(server_fd, &readfs)) {
            peerfd = accept(server_fd, (struct sockaddr*)&peer_addr, &peer_addr_size);
            ASSERT_LIBC(peerfd != -1, "accept");

            FILE* peer_fp = fdopen(peerfd, "r+");
            setlinebuf(peer_fp);

            handle_request(peer_fp);
            fclose(peer_fp);
        }
        if (FD_ISSET(fileno(in), &readfs)) {
            ASSERT_LIBC(getline(&line, &linecap, in) != EOF, "getline");
            UciCommand cmd;
            Result res = uci_parse_command(line, &cmd);
            if (res != RESULT_OK) {
                eout("invalid UCI command '%s'\n", line);
                eout("%s", get_error_msg(res));
            }
            if (cmd.kind == UCI_POSITION) {
                ui.game = cmd.position;
            } else if (cmd.kind == UCI_GO) {
                ui.waiting_move = true;
            } else if (cmd.kind == UCI_STOP) {
                ui.waiting_move = false;
            }
        }
    }

    close(server_fd);
    if (line) free(line);

    return 0;
}
