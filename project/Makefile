CC=gcc
# CFLAGS=-g -fsanitize=address
CFLAGS=

all: clean build

default: build

build: server.c client.c utils.c
	${CC} -o server server.c utils.c ${CFLAGS}
	${CC} -o client client.c utils.c ${CFLAGS}

clean:
	rm -rf server client *.bin *.out *.dSYM

zip: clean
	rm -f project0.zip
	mkdir -p project
	cp server.c client.c utils.c utils.h Makefile README.md project
	zip project0.zip project/*
