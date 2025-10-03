CC      = gcc
CFLAGS  = -Wall -g

# Source files
INFO_SRC      = mq_info.c
RECEIVER_SRC  = mq_receiver.c
SENDER_SRC    = mq_sender.c

# Object files
INFO_OBJ  = $(INFO_SRC:.c=.o)
RECEIVER_OBJ  = $(RECEIVER_SRC:.c=.o)
SENDER_OBJ  = $(SENDER_SRC:.c=.o)

# Targets
RECEIVER_BIN  = receiver
SENDER_BIN  = sender

.PHONY: all clean

all: receiver sender

receiver: $(RECEIVER_OBJ) $(INFO_OBJ)
	$(CC) $(CFLAGS) -o $(RECEIVER_BIN) $(RECEIVER_OBJ) $(INFO_OBJ)

sender: $(SENDER_OBJ) $(INFO_OBJ)
	$(CC) $(CFLAGS) -o $(SENDER_BIN) $(SENDER_OBJ) $(INFO_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(RECEIVER_BIN) $(SENDER_BIN)
