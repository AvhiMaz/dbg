.PHONY: program build fmt clean

program:
	cd program && cargo build-sbf

build:
	gcc -Wall -Wextra -g -fsanitize=address,undefined src/main.c -o dbg

fmt:
	clang-format -i $(wildcard src/*.c src/*.h)

clean:
	rm -f dbg && rm -rf program/target
