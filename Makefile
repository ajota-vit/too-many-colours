CFLAGS = -std=c99 -pedantic -Wall -Wextra
LDFLAGS = -lm

SRC = too_many_colours.c
OUT = too_many_colours

.PHONY: all run clean

all: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

run:
	./$(OUT)

clean:
	rm -f $(OUT)
