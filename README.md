# C Chess

Minimal chess machmaking server, with support for UI intergration.

## Dependencies

- C compiler, such as `gcc` or `clang`
- POSIX compliant operating system (Linux, MacOS, etc.)

## Building

```
make
mkdir games
```

## Running

### The machmaking server

```bash
./build/engine_chess games
```

### The ui server

```bash
./build/ui <color> games/<game_id>/<color>
```

Then you can open the browser at `http://localhost:8080` and play a game on the board.
