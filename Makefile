CFLAGS := -std=c11 -g -static

9cc: 9cc.c

debug: CFLAGS += -DDEBUG
debug: 9cc

test: 9cc
	./test.sh

clean:
	rm -f 9cc *.o *~ tmp*

.PHONY: test clean