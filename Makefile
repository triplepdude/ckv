CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2
DEBUG_FLAGS = -g -fsanitize=address,undefined

SRC   = src/hashmap.c src/storage.c
BIN   = ckv
TEST  = run_tests

.PHONY: all test debug clean

all: $(BIN)

$(BIN): $(SRC) src/main.c
	$(CC) $(CFLAGS) -o $@ $^

$(TEST): $(SRC) tests/test_ckv.c
	$(CC) $(CFLAGS) -o $@ $^

test: $(TEST)
	./$(TEST)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(BIN) $(TEST)
	./$(TEST)

clean:
	rm -f $(BIN) $(TEST) *.ckv *.ckv.tmp
