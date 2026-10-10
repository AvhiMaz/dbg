.PHONY: program build run clean

program:
	cd program && cargo build-sbf

build:
	gcc src/main.c -o main

run: build
	./main

clean:
	rm -f main && rm -rf program/target
