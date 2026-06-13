CC := gcc
CFLAGS := -g -std=c17 -Wall -Werror -Wextra -Iinclude
INCLUDE_DIR := include
SRC_DIR := src
BUILD_DIR := build

# external libraries
CFLAGS += -Ivendor/raylib/include
LIBS := -Lvendor/raylib/lib -lraylib -Wl,-rpath,vendor/raylib/lib -lm -lpthread -ldl -lGL -lX11

# headers
HDRS := $(shell find $(INCLUDE_DIR) -name '*.h')

# source files
SRCS := $(shell find $(SRC_DIR) -name '*.c')

# object files
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# executable
EXEC := bbcmicro

all: $(EXEC)

$(EXEC): $(OBJS) $(HDRS) Makefile
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXEC) $(OBJS)
