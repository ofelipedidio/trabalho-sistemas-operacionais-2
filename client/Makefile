CC=g++
CFLAGS=-Iinclude
DEPS=include/network.h include/file_manager.h include/app.h include/cli.h include/fs_notify.h
OBJ= src/network.cpp    src/file_manager.cpp   src/app.cpp   src/cli.cpp   src/fs_notify.cpp src/main.cpp

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

program: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

run: program 
	./program

clean:
	rm -f lex.yy.* *.o etapa* parser.tab.*
	rm -rf entrega

