CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./src
LDFLAGS = -lrt

.PHONY: all clean

all: 1_latency 2_mlock 3_benchmark

1_latency: src/1_latency.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

2_mlock: src/2_mlock.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

3_benchmark: src/3_benchmark.c src/mempool.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f 1_latency 2_mlock 3_benchmark