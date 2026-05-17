CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude
TARGET   = mips
SRC      = $(wildcard src/*.cpp)
OBJ      = $(SRC:.cpp=.o)

TEST_TARGET = mips_test
TEST_SRC    = $(wildcard tests/*.cpp) $(filter-out src/main.cpp, $(SRC))
TEST_OBJ    = $(TEST_SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) $(TEST_TARGET) src/*.o tests/*.o

.PHONY: all test clean
