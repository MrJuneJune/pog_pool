CC = gcc
CFLAGS = -Iinclude -lpq
BIN_DIR = bin
TEST_DIR = test
INCLUDE_DIR = include/pog_pool
SRC_DIR = src
BUILD_DIR = build

all: test debug

test: $(TEST_DIR)/benchmark_connection_pool.c $(BUILD_DIR)/connection.o | $(BIN_DIR)
	$(CC) $(TEST_DIR)/benchmark_connection_pool.c $(BUILD_DIR)/connection.o $(CFLAGS) -o $(BIN_DIR)/benchmark_pool
	./$(BIN_DIR)/benchmark_pool

# Need to use find since we create these models. I auto set it to build path.
debug: main.c auto_generate pog_pool 
	$(CC) main.c $(shell find $(BUILD_DIR) -name 'model_*.c')  $(CFLAGS) -Lbuild -lpog_pool -o $(BIN_DIR)/main

auto_generate: pog_pool | $(BIN_DIR)
	$(CC) generate_models.c $(CFLAGS) -Lbuild -lpog_pool -o $(BIN_DIR)/auto_generate
	$(BIN_DIR)/auto_generate

pog_pool: connection.o auto_generate.o
	ar rcs $(BUILD_DIR)/libpog_pool.a $(BUILD_DIR)/*.o

auto_generate.o: $(SRC_DIR)/auto_generate.c $(INCLUDE_DIR)/auto_generate.h | $(BUILD_DIR)
	$(CC) -c $(SRC_DIR)/auto_generate.c -o $(BUILD_DIR)/auto_generate.o $(CFLAGS)

connection.o: $(SRC_DIR)/connection.c $(INCLUDE_DIR)/connection.h | $(BUILD_DIR)
	$(CC) -c $(SRC_DIR)/connection.c -o $(BUILD_DIR)/connection.o $(CFLAGS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BIN_DIR) *.o $(LIB_DIR)/*.o
