
CFLAGS := -g
INCLUDE := -Isrc
SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,build/%.o,$(SRCS))
SVGS := $(wildcard svg/*.svg)
SVG_OBJS := $(patsubst svg/%.svg,build/svg/%.svg.o,$(SVGS))

all: build/engine_chess build/ui

build/engine_chess: bin/main.c $(OBJS) | build/
	gcc $(CFLAGS) $(INCLUDE) -o $@ $^

build/ui: bin/ui.c $(OBJS) $(SVG_OBJS) | build/
	gcc $(CFLAGS) $(INCLUDE) -o $@ $^

build/%.o: src/%.c | build/
	gcc $(CFLAGS) $(INCLUDE) -c -o $@ $^

build/svg/%.svg.o: build/svg/%.svg.c | build/svg/
	gcc $(CFLAGS) $(INCLUDE) -c -o $@ $^

build/svg/%.svg.c: svg/%.svg | build/svg/
	(cd svg; xxd -C -i $(patsubst svg/%.svg,%.svg,$<) ../$@)

%/:
	mkdir -p $@

clean:
	rm -rf build
