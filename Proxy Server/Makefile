CFLAGS =-Wall -Wextra -Werror -pedantic -pthread
LIBS =-lssl -lcrypto
CC=clang $(CFLAGS)
SOURCES = $(wildcard src/*.c)
EXECUTABLE = bin/myproxy

all: $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	$(CC) -o $@ $(SOURCES) $(LIBS)

clean:
	find bin -name "myproxy" -type f -delete
	find bin -name "myproxy.o" -type f -delete

infer:
	make clean
	infer-capture -- make
	infer-analyze -- make