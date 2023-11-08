CC=g++
CFLAGS=-Iinclude
DEPS=include/network.h include/filesystem.h include/app.h include/cli.h
OBJ=src/network.cpp    src/filesystem.h     src/app.h     src/cli.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ETAPA): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

run: $(ETAPA)
	./$(ETAPA)

clean:
	rm -f lex.yy.* *.o etapa* parser.tab.*
	rm -rf entrega

