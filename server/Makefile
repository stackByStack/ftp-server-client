# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -g

# Directories
SRC_DIR = .
UTILS_DIR = utils
BUILD_DIR = build

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
UTILS_SRCS = $(wildcard $(UTILS_DIR)/*.c)

# Object files
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
UTILS_OBJS = $(patsubst $(UTILS_DIR)/%.c,$(BUILD_DIR)/%.o,$(UTILS_SRCS))

# Executable name
TARGET = server

# Rule for building the executable
$(TARGET): $(OBJS) $(UTILS_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Rule for building object files from source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(UTILS_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Create the build directory if it doesn't exist
$(shell mkdir -p $(BUILD_DIR))

# Clean rule
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
