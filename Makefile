CC      = gcc
CFLAGS  = -Wall -Wextra -O2 $(shell pkg-config --cflags sdl2)
LDFLAGS = $(shell pkg-config --libs sdl2)

jeu_de_la_vie: jeu_de_la_vie.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f jeu_de_la_vie.exe jeu_de_la_vie

.PHONY: clean
