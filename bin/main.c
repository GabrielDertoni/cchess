#include <string.h>
#include <getopt.h>
#include <unistd.h>

#include "common.h"
#include "logging.h"
#include "game_server.h"
#include "fen.h"

#define MAX_GAME_LENGTH 512
#define MAX_DIR_LENGTH 512

static char fen[MAX_FEN_LENGTH + 1];
static char dir[MAX_DIR_LENGTH + 1];
static char game[MAX_GAME_LENGTH + 1];

char const* const DEFAULT_GAME = "game0";

void usage_exit(char* const progname) {
    fprintf(stderr, "Usage: %s [-f fen] [-g game] DIR\n", progname);
    exit(EXIT_FAILURE);
}

void parse_args(int argc, char* const argv[]) {
    static struct option const longopts[] = {
        {
            .name = "fen",
            .has_arg = true,
            .flag = NULL,
            .val = 0,
        },
        {
            .name = "game",
            .has_arg = true,
            .flag = NULL,
            .val = 0,
        },
        {0},
    };

    strcpy(fen, FEN_STARTING);
    strcpy(game, DEFAULT_GAME);

    int opt;
    while ((opt = getopt_long(argc, argv, "f:g:", longopts, NULL)) != -1) {
        switch (opt) {
            case 'f':
                strcpy(fen, optarg);
                break;

            case 'g':
                if (strlen(optarg) > MAX_GAME_LENGTH) {
                    fprintf(stderr, "error: game name too long");
                    usage_exit(argv[0]);
                }
                strcpy(game, optarg);
                break;

            default: /* '?' */
                usage_exit(argv[0]);
        }
    }
    if (optind >= argc) usage_exit(argv[0]);
    if (strlen(argv[optind]) + strlen(game) + 1 > MAX_DIR_LENGTH) {
        fprintf(stderr, "error: dir name too long");
        usage_exit(argv[0]);
    }
    strcpy(dir, argv[optind++]);
    strcat(dir, "/");
    strcat(dir, game);
    if (optind != argc) {
        fprintf(stderr, "error: unexpected arguments");
        usage_exit(argv[0]);
    }
}

int main(int argc, char* const argv[]) {
    setbuf(stdout, NULL);
    setbuf(stdin, NULL);

    parse_args(argc, argv);

    puts("~== ENGINE CHESS ==~");

    GameServer server;
    Result res = game_server_init_from_fen(&server, FEN_STARTING);
    if (res != RESULT_OK) {
        log_error("failed to initialize the game server");
        log_error("%s", get_error_msg(res));
        return EXIT_FAILURE;
    }

    char sbuf[2 * sizeof(dir) + 64];
    if (access(dir, F_OK) != 0) {
        sprintf(sbuf, "mkdir -p %s/white %s/black", dir, dir);
        if (system(sbuf) != 0) {
            log_error("failed to create directories with command '%s'", sbuf);
            return EXIT_FAILURE;
        }
        sprintf(sbuf, "mkfifo %s/white/in %s/white/out", dir, dir);
        if (system(sbuf) != 0) {
            log_error("failed to create fifos with command '%s'", sbuf);
            return EXIT_FAILURE;
        }
        sprintf(sbuf, "mkfifo %s/black/in %s/black/out", dir, dir);
        if (system(sbuf) != 0) {
            log_error("failed to create fifos with command '%s'", sbuf);
            return EXIT_FAILURE;
        }
    }

    FILE* fp;

    sprintf(sbuf, "%s/white/in", dir);
    fp = fopen(sbuf, "r+");
    if (!fp) {
        perror("fopen");
        return EXIT_FAILURE;
    }
    setbuf(fp, NULL);
    server.players[COLOR_WHITE].in = fp;

    sprintf(sbuf, "%s/white/out", dir);
    fp = fopen(sbuf, "w+");
    if (!fp) {
        perror("fopen");
        return EXIT_FAILURE;
    }
    setbuf(fp, NULL);
    server.players[COLOR_WHITE].out = fp;

    sprintf(sbuf, "%s/black/in", dir);
    fp = fopen(sbuf, "r+");
    if (!fp) {
        perror("fopen");
        return EXIT_FAILURE;
    }
    setbuf(fp, NULL);
    server.players[COLOR_BLACK].in = fp;

    sprintf(sbuf, "%s/black/out", dir);
    fp = fopen(sbuf, "w+");
    if (!fp) {
        perror("fopen");
        return EXIT_FAILURE;
    }
    setbuf(fp, NULL);
    server.players[COLOR_BLACK].out = fp;

    game_server_setup_uci(&server);
    do {
        res = game_server_update(&server);
    } while (res == RESULT_OK && !server.is_done);
    if (res != RESULT_OK) {
        log_error("%s", get_error_msg(res));
    }

    log_info("game finished");

    game_server_deinit(&server);

    return 0;
}
