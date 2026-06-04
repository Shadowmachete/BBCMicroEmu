CC := gcc
CFLAGS := -g -std=c17 -Wall -Werror -Wextra -Iinclude
INCLUDE_DIR := include
SRC_DIR := src
BUILD_DIR := build

# external libraries
# LIB_DIR := lib
# LIBC_ADD_DIR := $(LIB_DIR)/libc-additions
# LIBC_ADD_A := $(LIBC_ADD_DIR)/lib/libc-additions.a

# CFLAGS += -I$(LIBC_ADD_DIR)/include

# LDFLAGS :=
# LIBS := $(LIBC_ADD_A)

# headers
HDRS := $(shell find $(INCLUDE_DIR) -name '*.h')

# source files
SRCS := $(shell find $(SRC_DIR) -name '*.c')

# object files
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# executable
EXEC := bbcmicro

all: $(EXEC)

# $(LIBC_ADD_A):
# 	$(MAKE) -C $(LIBC_ADD_DIR)

$(EXEC): $(OBJS) $(HDRS) Makefile
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXEC) $(OBJS)

# distclean: clean
# 	$(MAKE) -C $(LIBC_ADD_DIR) clean
