BIN     = bin
OUT     = doto
INSTALL = /usr/bin/doto

SRC  = $(wildcard src/*.c)
DEPS = $(wildcard src/*.h)
OBJ  = $(addsuffix .o,$(subst src/,$(BIN)/,$(basename $(SRC))))

CSTD   = c99
CFLAGS = -O2 -std=$(CSTD) -D_POSIX_C_SOURCE -D_XOPEN_SOURCE -Wall -Wextra -Werror \
         -pedantic -Wno-deprecated-declarations -g -I./

build: $(OUT)

$(OUT): $(BIN) $(OBJ) $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(OBJ)

$(BIN)/%.o: src/%.c $(DEPS)
	$(CC) -c $< $(CFLAGS) -o $@

$(BIN):
	mkdir -p $(BIN)

install: $(OUT)
	cp $(OUT) $(INSTALL)

uninstall: $(INSTALL)
	rm $(INSTALL)

clean: $(BIN)
	rm $(OUT)
	rm -r $(BIN)/*

all:
	@echo build, install, uninstall, clean
