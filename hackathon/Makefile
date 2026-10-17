# Makefile for ProcessScope
# Zombie and Orphan Process Detection System
# Target: Ubuntu Linux / POSIX C11

CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -D_POSIX_C_SOURCE=200809L
LDFLAGS ?=

BIN_DIR = bin
OBJ_DIR = obj
SRC_DIR = src
TEST_DIR = tests

PROGS = $(BIN_DIR)/processcope \
        $(BIN_DIR)/zombie_demo \
        $(BIN_DIR)/orphan_demo \
        $(BIN_DIR)/test_parser

.PHONY: all clean test demo-zombie demo-orphan dashboard help

all: $(PROGS)

# Ensure output directories exist
$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

# Compile object files
$(OBJ_DIR)/scanner.o: $(SRC_DIR)/scanner.c $(SRC_DIR)/scanner.h $(SRC_DIR)/process.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/main.o: $(SRC_DIR)/main.c $(SRC_DIR)/scanner.h $(SRC_DIR)/process.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/test_parser.o: $(TEST_DIR)/test_parser.c $(SRC_DIR)/scanner.h $(SRC_DIR)/process.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link binaries
$(BIN_DIR)/processcope: $(OBJ_DIR)/main.o $(OBJ_DIR)/scanner.o | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/zombie_demo: $(TEST_DIR)/zombie_demo.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BIN_DIR)/orphan_demo: $(TEST_DIR)/orphan_demo.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BIN_DIR)/test_parser: $(OBJ_DIR)/test_parser.o $(OBJ_DIR)/scanner.o | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

test: all
	@echo "==> Running C Unit Tests..."
	@./$(BIN_DIR)/test_parser
	@echo "==> Running Automated Python Integration Suite..."
	@python3 tests/test_suite.py

demo-zombie: $(BIN_DIR)/zombie_demo
	@./$(BIN_DIR)/zombie_demo

demo-orphan: $(BIN_DIR)/orphan_demo
	@./$(BIN_DIR)/orphan_demo

dashboard: all
	@python3 dashboard/server.py

help:
	@echo "ProcessScope Build Targets:"
	@echo "  make              - Compile all binaries into ./bin/"
	@echo "  make clean        - Remove build artifacts and binaries"
	@echo "  make test         - Run parser unit tests and integration tests"
	@echo "  make demo-zombie  - Run interactive controlled zombie demonstration"
	@echo "  make demo-orphan  - Run controlled orphan reparenting demonstration"
	@echo "  make dashboard    - Start local dashboard server at http://127.0.0.1:5000"
