.PHONY: build fmt clean

CFLAGS := -Wall -Wextra -g -fsanitize=address,undefined
SRCS   := $(wildcard src/*.c)
HDRS   := $(wildcard src/*.h)

build:
	gcc $(CFLAGS) $(SRCS) -o dbg

fmt:
	clang-format -i $(SRCS) $(HDRS)

clean:
	rm -f dbg
