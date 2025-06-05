CC = gcc
CFLAGS = -Ilib -L. -lpq
BIN_DIR = bin
TEST_DIR = test
LIB_DIR = lib
MODEL_SRCS := $(wildcard $(LIB_DIR)/model_*.c)

all: test debug

test: $(TEST_DIR)/benchmark_connection_pool.c $(LIB_DIR)/connection.o | $(BIN_DIR)
	$(CC) $(TEST_DIR)/benchmark_connection_pool.c $(LIB_DIR)/connection.o $(CFLAGS) -o $(BIN_DIR)/benchmark_pool
	./$(BIN_DIR)/benchmark_pool

debug: auto_generate_sql main.c $(LIB_DIR)/connection.o | $(BIN_DIR)
	$(CC) main.c $(MODEL_SRCS) $(LIB_DIR)/connection.o $(CFLAGS) -o $(BIN_DIR)/main

auto_generate_sql: $(LIB_DIR)/auto_generate.c $(LIB_DIR)/auto_generate.h
	$(CC) $(LIB_DIR)/auto_generate.c -o $(BIN_DIR)/auto_generate $(CFLAGS) && ./$(BIN_DIR)/auto_generate

lib/connection.o: $(LIB_DIR)/connection.c $(LIB_DIR)/connection.h
	$(CC) -c $(LIB_DIR)/connection.c -o $(LIB_DIR)/connection.o $(CFLAGS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR) *.o $(LIB_DIR)/*.o
