# Makefile for FFmpeg project

# Compiler
CC = gcc

# Output executable
TARGET = main

# Source files
SRCS = main.c

# Compiler flags
CFLAGS = -Wall -O2

# Libraries to link
LIBS = -lavcodec -lavutil -lavformat -lswscale -lm

# Build target
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LIBS)

# Run the program with optional arguments
run: $(TARGET)
	./$(TARGET) 'Bruh sound effect'.mp4

# Clean build files
clean:
	rm -f $(TARGET) *.o

.PHONY: all clean run

