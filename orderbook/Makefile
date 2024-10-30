.PHONY: all clean build run test

BUILD_DIR := build

all: build

clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)

test:
	@cd build && ctest --output-on-failure

build:
	@echo "Building project..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake ..
	@cd $(BUILD_DIR) && cmake --build .

run: build
	@echo "Running project..."
	@./$(BUILD_DIR)/client