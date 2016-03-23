CC = clang
CFLAGS = -std=c99 -Wall -g

TARGETS = nc125
all: $(TARGETS)

nc125: nc125.c

clean:
	rm -rf $(TARGETS) *.dSYM
