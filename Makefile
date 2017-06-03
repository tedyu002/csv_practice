CC=gcc
CFLAGS=-Wall -std=c99 -Wpedantic


csv_practice: src/main.o
	${CC} src/main.o -o csv_practice

.PHONY: clean
clean:
	@rm -rf src/*.o
	@rm -rf csv_practice
