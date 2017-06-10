LIBS=src/mmap.o src/config.o src/token.o src/type.o src/csv.o src/val.o src/container.o src/op.o
INCLUDE=

CC=gcc
CVER=-std=c99
#ASAN=-fsanitize=address -lasan
#FLTO="-flto"
DEFS=-D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -D_GNU_SOURCE
LDFLAGS=${FLTO} ${ASAN}
CFLAGS=-Wall ${LDFLAGS} -g ${CVER} -Werror -Wpedantic ${INCLUDE} ${DEFS}

csv_practice: src/main.o ${LIBS}
	${CC} ${CFLAGS} src/main.o ${LIBS} -o csv_practice

test: src/test.c
	${CC} ${DEFS} ${CVER} -Werror -Wpedantic  src/test.c -o csv_practice_test


.PHONY: clean
clean:
	@rm -rf src/*.o
	@rm -f csv_practice
	@rm -f csv_practice_test
