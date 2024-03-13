CC = gcc

.PHONY: all
all: hello


hello: hello.c
	$(CC) hello.c -o hello

.PHONY: run
run:
	./hello

.PHONY: clean
clean:
	rm hello

