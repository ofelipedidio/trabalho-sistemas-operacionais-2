CXX = g++
# -Wall: Generate all warning
# -g: Keep debugging symbols
CXXFLAGS = -Wall -g

# Project files
SRC = $(wildcard src/*.cpp)
OBJ = $(patsubst src/%.cpp, build/%.o, $(SRC))

# The first rule is the one that is ran when you run `make`
all: clean compile 

# Create directories
bin:
	mkdir -p bin

build:
	mkdir -p build

# Compile `.cpp` to `.o`
build/%.o: src/%.cpp include/%.h |build
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile `.o` to binary
bin/server: $(OBJ) |bin
	$(CXX) -o bin/server $(OBJ)

# Rules that are not attached to files
.PHONY: compile clean test

# Targets
compile: bin/server

test: CXXFLAGS += -DTEST_MODE
test: clean compile 
	./bin/server

clean:
	rm -rf build/* bin/server


