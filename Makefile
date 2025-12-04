# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Source files
SRCS = main.c init.c add.c commit.c log.c checkout.c branch.c merge.c diff.c remote.c utils.c

# Object files
OBJS = $(SRCS:.c=.o)

# Output executable
TARGET = mini_git

# Default target
all: $(TARGET)

# Link executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile .c files to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean object files and executable (Windows-compatible version)
clean:
	del /f /q $(OBJS) $(TARGET)
