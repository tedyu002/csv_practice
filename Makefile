LIBS=src/mmap.o src/config.o src/token.o src/type.o src/csv.o src/val.o src/container.o src/op.o
INCLUDE=-Isrc

CC=gcc
#ASAN=-fsanitize=address -lasan
#FLTO="-flto"
LDFLAGS=${FLTO} ${ASAN}
CFLAGS=-Wall ${LDFLAGS} -g -std=c99 -Werror -Wpedantic ${INCLUDE} -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -D_GNU_SOURCE

csv_practice: src/main.o ${LIBS}
	${CC} ${CFLAGS} src/main.o ${LIBS} -o csv_practice

test: test_config_column_print test_token_string_parse test_csv_row_parse test_csv_csv_parse \
		test_sort_sort test_cal_cal

test_config_column_print: test_src/config/column_print.o ${LIBS} test_dir
	${CC} ${CLFAGS} ${LDFLAGS} test_src/config/column_print.o ${LIBS} -o test/config_column_print

test_token_string_parse: test_src/token/string_parse.o ${LIBS} test_dir
	${CC} ${CLFAGS} ${LDFLAGS} test_src/token/string_parse.o ${LIBS} -o test/toekn_string_parse

test_csv_row_parse: test_src/csv/row_parse.o ${LIBS} test_dir
	${CC} ${CFLAGS} ${LDFLAGS} test_src/csv/row_parse.o ${LIBS} -o test/csv_row_parse

test_csv_csv_parse: test_src/csv/csv_parse.o ${LIBS} test_dir
	${CC} ${CFLAGS} ${LDFLAGS} test_src/csv/csv_parse.o ${LIBS} -o test/csv_csv_parse

test_sort_sort: test_src/sort/sort.o ${LIBS} test_dir
	${CC} ${CFLAGS} ${LDFLAGS} test_src/sort/sort.o ${LIBS} -o test/sort_sort

test_cal_cal: test_src/cal/cal.o ${LIBS} test_dir
	${CC} ${CFLAGS} ${LDFLAGS} test_src/cal/cal.o ${LIBS} -o test/cal_cal

test_dir:
	@mkdir -p test

.PHONY: clean
clean:
	@rm -rf src/*.o
	@rm -rf csv_practice
	@rm -rf test/
