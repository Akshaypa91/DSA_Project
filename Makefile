# Compiler
CC = gcc
CFLAGS = -Wall -g

# Executable name
TARGET = mini_git

# Source files
SRCS = main.c init.c add.c commit.c log.c checkout.c branch.c merge.c diff.c remote.c utils.c
OBJS = $(SRCS:.c=.o)

# Build target
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile .c into .o
%.o: %.c utils.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJS) $(TARGET)

# Run program
run: $(TARGET)
	./$(TARGET)
