.PHONY: build fmt check clean

CFLAGS := -Wall -Wextra -g -fsanitize=address,undefined
SRCS   := $(wildcard src/*.c)
HDRS   := $(wildcard src/*.h)

build:
	gcc $(CFLAGS) $(SRCS) -o dbg

fmt:
	clang-format -i $(SRCS) $(HDRS)

check: build
	bash scripts/compare.sh

clean:
	rm -f dbg
