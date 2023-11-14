CC=g++
CFLAGS=-Iinclude
DEPS=include/network.h include/file_manager.h include/app.h include/cli.h include/fs_notify.h include/task_queue.h
OBJ= src/network.cpp    src/file_manager.cpp   src/app.cpp   src/cli.cpp   src/fs_notify.cpp
TEST=test/test.cpp
BUILD=src/main.cpp

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

%.o: %.c $(BUILD)
	$(CC) -c -o $@ $< $(CFLAGS)

%.o: %.c $(TEST)
	$(CC) -c -o $@ $< $(CFLAGS)


program: $(OBJ) $(BUILD)
	$(CC) -o $@ $^ $(CFLAGS)

run: program 
	./program

test_build: $(OBJ) $(TEST)
	$(CC) -o test_thing $^ $(CFLAGS)

test: test_build
	./test_thing

clean:
	rm -f lex.yy.* *.o etapa* parser.tab.*
	rm -rf entrega

