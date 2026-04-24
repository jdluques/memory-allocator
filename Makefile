BUILD_DIR := build
TYPE      := Debug

.PHONY: all build test clean rebuild valgrind

all: build

build:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(TYPE) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build $(BUILD_DIR)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean build

valgrind: build
	valgrind --leak-check=full --track-origins=yes $(BUILD_DIR)/test_stress
