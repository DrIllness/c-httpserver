# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -std=c11 -g

# Source files
SRCS = server.c request.c response.c queue.c threadpool2.c logger.c

# Object files
OBJS = $(SRCS:.c=.o)

# Target executable
TARGET = server

# Default rule
all: $(TARGET)

# Rule to link object files into the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Rule to compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule to remove compiled files
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
