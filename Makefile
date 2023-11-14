CXX = g++
CXXFLAGS = -Wall -g

# Project files
CLIENT_SRCS = $(wildcard client/src/*.cpp)
CLIENT_OBJS = $(patsubst client/src/%.cpp, build/client/%.o, $(CLIENT_SRCS))

SERVER_SRC = $(wildcard server/src/*.cpp)
SERVER_OBJ = $(patsubst server/src/%.cpp, build/server/%.o, $(SERVER_SRCS))

# The first rule is the one that is ran when you run `make`
all: clean client server

# Create directories
bin:
	mkdir -p bin

build/client:
	mkdir -p build build/client

build/server:
	mkdir -p build/server

# Compile `.cpp` to `.o`
build/client/%.o: client/src/%.cpp |build/client
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/server/%.o: server/src/%.cpp |build/server
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile `.o` to binary
bin/client: $(CLIENT_OBJS) |bin
	$(CXX) -o bin/client $(CLIENT_OBJS)

bin/server: $(SERVER_OBJS) |bin
	$(CXX) -o bin/client $(SERVER_OBJS)

# Rules that are not attached to files
.PHONY: client server clean

# Targets
client: bin/client

server: bin/server

test_client: CXXFLAGS += -DTEST_MODE
test_client: client

test_server: CXXFLAGS += -DTEST_MODE
test_server: client

clean:
	rm -rf build bin
