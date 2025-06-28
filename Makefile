CC = gcc
INCLUDE_CFLAGS = -I./third_party/include -I./include
LIBRARY_CFLAG  = -L./build -L./third_party/lib
CFLAGS := $(INCLUDE_CFLAGS) $(LIBRARY_CFLAG)

BIN_DIR = bin
TEST_DIR = test
INCLUDE_DIR = include/pog_pool
SRC_DIR = src
BUILD_DIR = build
TEMPLATE_DIR = templates

THIRD_PARTY_PREFIX := $(CURDIR)/third_party
THIRD_PARTY_INCLUDE_DIR := $(THIRD_PARTY_PREFIX)/include
THIRD_PARTY_LIB_DIR := $(THIRD_PARTY_PREFIX)/lib

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  POSTGRES_SRC := install_libpq_mac
  SHARED_LIB := $(BUILD_DIR)/libpog_pool.dylib
  DYNAMICALLY_PATH := -Wl,-install_name,$(CURDIR)/build/libpog_pool.dylib
  SHARED_FLAGS := -dynamiclib -Iinclude 
else
  POSTGRES_SRC := install_libpq_linux
  SHARED_LIB := $(BUILD_DIR)/libpog_pool.so
  DYNAMICALLY_PATH := -Wl,-rpath,$(CURDIR)/build
  SHARED_FLAGS := -shared -Wl $(DYNAMICALLY_PATH) -Iinclude 
endif

all: debug

release: pog_pool 
	cp -r include/* dist/include
	cp $(BUILD_DIR)/libpog_pool.a dist 

bench_mark: pog_pool_benchmark python_benchmark

pog_pool_benchmark: pog_pool $(TEST_DIR)/benchmark_connection_pool.c $(BUILD_DIR)/connection.o | $(BIN_DIR)
	$(CC) $(TEST_DIR)/benchmark_connection_pool.c $(BUILD_DIR)/connection.o $(CFLAGS) \
		-lpog_pool -lpq  \
		-o $(BIN_DIR)/benchmark_pool
	./$(BIN_DIR)/benchmark_pool

python_benchmark:
ifndef NO_VENV
	@if [ ! -d "venv" ]; then \
		echo "Creating virtual environment..."; \
		python3 -m venv venv; \
		. venv/bin/activate && pip install -r test/requirement.txt; \
	else \
		echo "Using existing virtual environment..."; \
	fi
	. venv/bin/activate && python3 test/sqlalchemy_benchmark.py
else
	python3 test/sqlalchemy_benchmark.py
endif

example: example/main.c auto_generate 
	$(CC) example/main.c $(shell find $(BUILD_DIR) -name 'model_*.c') \
		$(CFLAGS) \
		-lpog_pool -lpq  \
  	-o $(BIN_DIR)/main
	./$(BIN_DIR)/main

auto_generate: pog_pool | $(BIN_DIR)
	$(CC) example/generate_models.c $(CFLAGS) \
		-lpog_pool -lpq  \
		-o $(BIN_DIR)/auto_generate
	cd example && ../$(BIN_DIR)/auto_generate

pog_pool: $(SHARED_LIB) $(BUILD_DIR)/libpog_pool.a

$(SHARED_LIB): $(BUILD_DIR)/connection.o $(BUILD_DIR)/auto_generate.o
	$(CC) -dynamiclib $(CFLAGS) -lpq \
    $(DYNAMICALLY_PATH)\
    -o $@ \
		$^

$(BUILD_DIR)/libpog_pool.a: $(BUILD_DIR)/connection.o $(BUILD_DIR)/auto_generate.o
	ar rcs $@ $^

$(BUILD_DIR)/auto_generate.o: $(SRC_DIR)/auto_generate.c $(INCLUDE_DIR)/auto_generate.h template_to_header install_libpq | $(BUILD_DIR)
	$(CC) -fPIC -c $(SRC_DIR)/auto_generate.c -o $@ $(CFLAGS) -lpq

$(BUILD_DIR)/connection.o: $(SRC_DIR)/connection.c $(INCLUDE_DIR)/connection.h install_libpq | $(BUILD_DIR)
	$(CC) -fPIC -c $(SRC_DIR)/connection.c -o $@ $(CFLAGS) -lpq

template_to_header: $(SRC_DIR)/$(TEMPLATE_DIR)/crud_header.pog_templ $(SRC_DIR)/$(TEMPLATE_DIR)/crud.pog_templ
	echo 'char* CRUD_HEADER_TEMPLATE =' > $(INCLUDE_DIR)/crud_header_template.h
	sed 's/\\/\\\\/g; s/"/\\"/g; s/^/"/; s/$$/\\n"/' $(SRC_DIR)/$(TEMPLATE_DIR)/crud_header.pog_templ >> $(INCLUDE_DIR)/crud_header_template.h
	echo ';' >> $(INCLUDE_DIR)/crud_header_template.h
	echo 'char* CRUD_SRC_TEMPLATE =' > $(INCLUDE_DIR)/crud_src_template.h
	sed 's/\\/\\\\/g; s/"/\\"/g; s/^/"/; s/$$/\\n"/' $(SRC_DIR)/$(TEMPLATE_DIR)/crud.pog_templ >> $(INCLUDE_DIR)/crud_src_template.h
	echo ';' >> $(INCLUDE_DIR)/crud_src_template.h

install_libpq:
	@if [ -d $(THIRD_PARTY_INCLUDE_DIR)/postgresql ] && [ -d  $(THIRD_PARTY_INCLUDE_DIR) ]; then \
		echo "Postgres already installed."; \
	else \
		read -p "Do you want to install postgresql via homebrew or apt? [y/N] " answer; \
		if [ "$$answer" = "y" ] || [ "$$answer" = "Y" ]; then \
		  $(MAKE) $(POSTGRES_SRC); \
		else \
			echo "Skip installations. Please move libpq yourself."; \
		fi \
	fi

install_libpq_mac: prepare_dirs
	@echo "Installing libpq via Homebrew..."
	brew install libpq 
	@echo "Copying libpq and libs to third_party..."
	cp -r /opt/homebrew/opt/libpq/include/* $(THIRD_PARTY_INCLUDE_DIR)/postgresql/
	cp -r /opt/homebrew/opt/libpq/lib/* $(THIRD_PARTY_LIB_DIR)/

install_libpq_linux: prepare_dirs
	@echo "Installing libpq via apt..."
	sudo apt-get update
	sudo apt-get install -y libpq 
	@echo "Copying libpq headers and libs to third_party..."
	cp -r /usr/include/postgresql/* $(THIRD_PARTY_INCLUDE_DIR)/postgresql/
	cp -r /usr/lib/*libpq.* $(THIRD_PARTY_LIB_DIR)/

prepare_dirs:
	mkdir -p $(THIRD_PARTY_INCLUDE_DIR)/postgresql
	mkdir -p $(THIRD_PARTY_LIB_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR) dist
