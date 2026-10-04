# CALQL8 — host build
# The engine (src/) is platform-independent C++; these unit tests compile
# and run on the host with a plain g++ toolchain. The Teensy firmware
# (src/main.cpp) is built separately via PlatformIO — see docs/BUILD.md.

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Isrc
TEST_BIN := build/test_engine

.PHONY: test clean

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): test/test_engine.cpp src/clock_engine.h src/sequencer.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ test/test_engine.cpp

clean:
	rm -rf build
