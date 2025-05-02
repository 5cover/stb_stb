# Makefile for stb_stb with separate bin/ and obj/ directories

# Directories
SRC_DIR = src
LIB_DIR = $(SRC_DIR)/lib
OBJ_DIR = obj
BIN_DIR = bin

# Output binary
BIN = $(BIN_DIR)/stb_stb

# Source files
C_SRCS = \
	$(SRC_DIR)/stb_stb.c \
	$(LIB_DIR)/memlst.c \
	$(LIB_DIR)/stb_ds.c

# Object files (mirror directory structure)
OBJS = $(C_SRCS:.c=.o)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(OBJS))
OBJS := $(patsubst $(LIB_DIR)/%.c, $(OBJ_DIR)/lib/%.o, $(OBJS))

# Compiler and flags
CC = cc
CFLAGS = -std=c99 -Wall -Wextra -Werror

# Default target
all: $(BIN)

# Create bin and obj directories if needed
$(BIN): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Pattern rule to compile .c into .o in obj/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)/%
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/lib/%.o: $(LIB_DIR)/%.c | $(OBJ_DIR)/lib
	$(CC) $(CFLAGS) -c $< -o $@

# Directory creation rules
$(OBJ_DIR)/%:
	mkdir -p $(dir $@)

$(OBJ_DIR)/lib:
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
