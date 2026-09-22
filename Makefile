CC      = gcc
CFLAGS  = -Wall -Wextra -O2 $(shell pkg-config --cflags raylib)
LDFLAGS = $(shell pkg-config --libs raylib) -lopengl32 -lgdi32 -lwinmm

# Sous MSYS2, make vide les variables TMP/TEMP avant de lancer gcc, qui se
# rabat alors sur C:\Windows et n'a pas le droit d'y ecrire. On lui redonne
# un dossier temporaire valide, uniquement si l'environnement n'en a pas.
SAFETMP := $(shell cygpath -w /tmp 2>/dev/null)
FIXTMP   = $(if $(SAFETMP),TMP="$${TMP:-$(SAFETMP)}" TEMP="$${TEMP:-$(SAFETMP)}",)

jeu_de_la_vie: jeu_de_la_vie.c
	$(FIXTMP) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f jeu_de_la_vie.exe jeu_de_la_vie

.PHONY: clean
