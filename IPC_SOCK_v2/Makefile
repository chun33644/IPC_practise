CC      = gcc
CFLAGS  = -Wall -g

# Source files
GENERAL_SRC  = IPC_SOCK_general.c
CLIENT_SRC   = IPC_SOCK_client.c
SERVER_SRC   = IPC_SOCK_server.c

# Object files
GENERAL_OBJ  = $(GENERAL_SRC:.c=.o)
CLIENT_OBJ   = $(CLIENT_SRC:.c=.o)
SERVER_OBJ   = $(SERVER_SRC:.c=.o)

# Targets
CLIENT_BIN   = IPC_SOCK_client
SERVER_BIN   = IPC_SOCK_server


.PHONY: all clean

all: client server

client: $(CLIENT_OBJ) $(GENERAL_OBJ)
	$(CC) $(CFLAGS) -o $(CLIENT_BIN) $(CLIENT_OBJ) $(GENERAL_OBJ)

server: $(SERVER_OBJ) $(GENERAL_OBJ)
	$(CC) $(CFLAGS) -o $(SERVER_BIN) $(SERVER_OBJ) $(GENERAL_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(CLIENT_BIN) $(SERVER_BIN)
	rm socketUDS
