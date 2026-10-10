.PHONY: program build run clean

program:
	cd program && cargo build-sbf

build:
	gcc -Wall -Wextra -g -fsanitize=address,undefined src/main.c -o main

run: build
	./main

clean:
	rm -f main && rm -rf program/target
