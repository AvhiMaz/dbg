.PHONY: program build clean

program:
	cd program && cargo build-sbf

build:
	gcc -Wall -Wextra -g -fsanitize=address,undefined src/main.c -o dbg

clean:
	rm -f dgb && rm -rf program/target
