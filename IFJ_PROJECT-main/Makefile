# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pedantic

# Automatically collect all .c files in the current directory
SRC = $(wildcard *.c)

# Output executable name
TARGET = main

# Default rule
all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

# Clean up the compiled executable
clean:
	rm -f $(TARGET)

run:
	./main < finallextest.ifj25

.PHONY: all clean
