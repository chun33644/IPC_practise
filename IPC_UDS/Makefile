CC      = gcc
CFLAGS  = -Wall -g

# Source files
COMMON_SRC  = IPC_UDS_common.c
CLIENT_SRC  = IPC_UDS_client.c
SERVER_SRC  = IPC_UDS_server.c

# Object files
COMMON_OBJ  = $(COMMON_SRC:.c=.o)
CLIENT_OBJ  = $(CLIENT_SRC:.c=.o)
SERVER_OBJ  = $(SERVER_SRC:.c=.o)

# Targets
CLIENT_BIN  = IPC_UDS_client
SERVER_BIN  = IPC_UDS_server

.PHONY: all clean

all: client server

client: $(CLIENT_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o $(CLIENT_BIN) $(CLIENT_OBJ) $(COMMON_OBJ)

server: $(SERVER_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o $(SERVER_BIN) $(SERVER_OBJ) $(COMMON_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(CLIENT_BIN) $(SERVER_BIN)
