CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wpedantic -pthread

.PHONY: all run test tsan clean
all: build/bank_demo

build:
	mkdir -p build

build/bank_demo: src/main.cpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

build/bank_tests: tests/bank_tests.cpp src/main.cpp | build
	$(CXX) $(CXXFLAGS) tests/bank_tests.cpp -o $@

run: build/bank_demo
	./build/bank_demo

test: build/bank_tests
	./build/bank_tests

tsan: | build
	$(CXX) -std=c++17 -O1 -g -fsanitize=thread -fno-omit-frame-pointer -pthread tests/bank_tests.cpp -o build/bank_tests_tsan
	./build/bank_tests_tsan

clean:
	rm -rf build
