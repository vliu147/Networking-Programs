CFLAGS = -Wall -Wextra -Werror -pedantic
CC=clang $(CFLAGS)
SOURCES = $(wildcard src/*.c)
EXECUTABLE = bin/myweb

all: $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	$(CC) -o $@ $(SOURCES)
	

clean:
	find bin -name "myweb" -type f -delete
	find bin -name "myweb.o" -type f -delete

infer:
	make clean
	infer-capture -- make
	infer-analyze -- make