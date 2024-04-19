CFLAGS =-Wall -Wextra -Werror -pedantic -pthread
CC=clang $(CFLAGS)

# Directories
SRC_DIR = src
BIN_DIR = bin

# Source and object files
CLIENT_SRC = $(SRC_DIR)/myclient.c
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_SRC = $(SRC_DIR)/myserver.c
SERVER_OBJ = $(SERVER_SRC:.c=.o)

# Targets
all: client server

client: $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/myclient $^

server: $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/myserver $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(BIN_DIR)/* $(SRC_DIR)/*.o