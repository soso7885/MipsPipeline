SRC = sim.c linkedlist.c
CC = gcc
CFLAGS = -std=c99 -g

TARGET = sim

all: $(TARGET)

sim: $(SRC)
	$(CC) -Wall $(CFLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET)
