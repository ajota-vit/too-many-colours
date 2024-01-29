CFLAGS = -std=c99 -pedantic -Wall -Wextra
LDFLAGS = -lm

.PHONY: all run clean

all: $(SRC)
	$(CC) $(CFLAGS) too_many_colours.c -o tmc $(LDFLAGS)
	$(CC) $(CFLAGS) gradient.c -o gradient $(LDFLAGS)

clean:
	rm -f tmc gradient
