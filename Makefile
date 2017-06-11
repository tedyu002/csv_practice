LIBS=src/mmap.o src/config.o src/token.o src/type.o src/csv.o src/val.o src/container.o src/op.o
INCLUDE=

CC=gcc
CVER=-std=c99
#ASAN=-fsanitize=address -lasan
#FLTO="-flto"
DEFS=-D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -D_GNU_SOURCE
LDFLAGS=${FLTO} ${ASAN}
CFLAGS=-Wall ${LDFLAGS} -g ${CVER} -Werror -Wpedantic ${INCLUDE} ${DEFS}

all: csv_practice csv_test csv_generator

csv_practice: src/main.o ${LIBS}
	${CC} ${CFLAGS} src/main.o ${LIBS} -o csv_practice

csv_test: src/main_test.o
	${CC} ${CFLAGS}  src/main_test.o -o csv_test

csv_generator: src/main_generator.o ${LIBS}
	${CC} ${CFLAGS} src/main_generator.o ${LIBS} -o csv_generator

.PHONY: clean
clean:
	@rm -rf src/*.o
	@rm -f csv_practice csv_test csv_generator
