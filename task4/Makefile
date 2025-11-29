CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGETS = read_input poll_inputs

.PHONY: all clean

all: $(TARGETS)

read_input: src/read_inputs.c
	$(CC) $(CFLAGS) -o $@ $^

poll_inputs: src/poll_inputs.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS)

