CC = gcc
CFLAGS = -Iinclude -lpq
BIN_DIR = bin
TEST_DIR = test
INCLUDE_DIR = include/pog_pool
SRC_DIR = src
BUILD_DIR = build
TEMPLATE_DIR = templates

all: debug

bench: pog_pool_benchmark python_benchmark

pog_pool_benchmark: $(TEST_DIR)/benchmark_connection_pool.c $(BUILD_DIR)/connection.o | $(BIN_DIR)
	$(CC) $(TEST_DIR)/benchmark_connection_pool.c $(BUILD_DIR)/connection.o $(CFLAGS) -o $(BIN_DIR)/benchmark_pool
	./$(BIN_DIR)/benchmark_pool

python_benchmark:
ifndef NO_VENV
	@if [ ! -d "venv" ]; then \
		echo "Creating virtual environment..."; \
		python3 -m venv venv; \
		. venv/bin/activate && pip install -r test/requirements.txt; \
	else \
		echo "Using existing virtual environment..."; \
	fi
	. venv/bin/activate && python3 test/sqlalchemy_benchmark.py
else
	python3 test/sqlalchemy_benchmark.py
endif

# Need to use find since we create these models. I auto set it to build path.
example: example/main.c auto_generate pog_pool 
	$(CC) example/main.c $(shell find $(BUILD_DIR) -name 'model_*.c')  $(CFLAGS) -Lbuild -lpog_pool -o $(BIN_DIR)/main

auto_generate: pog_pool | $(BIN_DIR)
	$(CC) example/generate_models.c $(CFLAGS) -Lbuild -lpog_pool -o $(BIN_DIR)/auto_generate
	$(BIN_DIR)/auto_generate

pog_pool: connection.o auto_generate.o 
	ar rcs $(BUILD_DIR)/libpog_pool.a $(BUILD_DIR)/*.o

auto_generate.o: $(SRC_DIR)/auto_generate.c $(INCLUDE_DIR)/auto_generate.h template_to_header | $(BUILD_DIR)
	$(CC) -c $(SRC_DIR)/auto_generate.c -o $(BUILD_DIR)/auto_generate.o $(CFLAGS)

connection.o: $(SRC_DIR)/connection.c $(INCLUDE_DIR)/connection.h | $(BUILD_DIR)
	$(CC) -c $(SRC_DIR)/connection.c -o $(BUILD_DIR)/connection.o $(CFLAGS)

template_to_header: $(SRC_DIR)/$(TEMPLATE_DIR)/crud_header.pog_templ $(SRC_DIR)/$(TEMPLATE_DIR)/crud.pog_templ
	echo 'char* CRUD_HEADER_TEMPLATE =' > $(INCLUDE_DIR)/crud_header_template.h
	sed 's/\\/\\\\/g; s/"/\\"/g; s/^/"/; s/$$/\\n"/' $(SRC_DIR)/$(TEMPLATE_DIR)/crud_header.pog_templ >> $(INCLUDE_DIR)/crud_header_template.h
	echo ';' >> $(INCLUDE_DIR)/crud_header_template.h
	echo 'char* CRUD_SRC_TEMPLATE =' > $(INCLUDE_DIR)/crud_src_template.h
	sed 's/\\/\\\\/g; s/"/\\"/g; s/^/"/; s/$$/\\n"/' $(SRC_DIR)/$(TEMPLATE_DIR)/crud.pog_templ >> $(INCLUDE_DIR)/crud_src_template.h
	echo ';' >> $(INCLUDE_DIR)/crud_src_template.h

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BIN_DIR) *.o $(LIB_DIR)/*.o
