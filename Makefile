ifeq ($(origin CC), default)
CC := gcc
endif
ifeq ($(origin AR), default)
AR := ar
endif

CPPFLAGS := -Iinclude
CFLAGS := -std=c11 -Wall -Wextra -Werror -Wpedantic -O2 -fPIC

NAME := hashmap
SRC := src/hashmap.c src/hash.c
OBJ := $(SRC:.c=.o)

TEST_SRC := tests/test_hashmap.c
TEST_BIN := tests/hashmap_tests
TEST_SAN_BIN := tests/hashmap_tests_sanitize

all: static shared

static: lib$(NAME).a

shared: lib$(NAME).so

lib$(NAME).a: $(OBJ)
	$(AR) rcs $@ $^

lib$(NAME).so: $(OBJ)
	$(CC) -shared -o $@ $^

%.o: %.c include/hashmap.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_BIN): $(TEST_SRC) $(SRC) include/hashmap.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -O0 -g3 $(TEST_SRC) $(SRC) -o $@

$(TEST_SAN_BIN): $(TEST_SRC) $(SRC) include/hashmap.h
	$(CC) $(CPPFLAGS) -std=c11 -Wall -Wextra -Werror -Wpedantic \
		-O1 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined \
		$(TEST_SRC) $(SRC) -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

test-sanitize: $(TEST_SAN_BIN)
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
	UBSAN_OPTIONS=halt_on_error=1 \
	HASHMAP_TEST_NO_FORK=1 \
	./$(TEST_SAN_BIN)

test-ci: test test-sanitize

clean:
	rm -f $(OBJ) $(TEST_BIN) $(TEST_SAN_BIN)

fclean: clean
	rm -f lib$(NAME).a lib$(NAME).so

re: fclean all

.PHONY: all static shared test test-sanitize test-ci clean fclean re
