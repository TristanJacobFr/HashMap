CC = gcc
AR = ar

CFLAGS = -Wall -Werror -Wextra -o2 -fPIC
INC = -Iinclude

SRC = src/hashmap.c src/hash.c
OBJ = $(SRC:.c=.o)

NAME = hashmap

all: static shared

static: lib$(NAME).a
shared: lib$(NAME).o

lib$(NAME).a: $(OBJ)
	$(AR) rcs $@ $^

lib$(NAME).o: $(OBJ)
	$(CC) -shared -o $@ $^

%.o: %.c
	$(CC) $(INC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f lib$(NAME).a lib$(NAME).o

re: fclean all

.PHONY: all static shared clean fclean re