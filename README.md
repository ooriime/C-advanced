# Jeu de la vie (raylib)

Le jeu de la vie de Conway, en C avec raylib.

## Compilation

Dans un terminal MSYS2 **UCRT64** :

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-raylib   # une seule fois
make
```

Sans Makefile :

```sh
gcc -Wall -Wextra -O2 $(pkg-config --cflags raylib) \
    -o jeu_de_la_vie.exe jeu_de_la_vie.c \
    $(pkg-config --libs raylib) -lopengl32 -lgdi32 -lwinmm
```

## Lancement

```sh
./jeu_de_la_vie.exe
```

## Commandes

| Touche                | Effet                                   |
|-----------------------|-----------------------------------------|
| Clic gauche / glisser | Donner la vie a une cellule             |
| Clic droit / glisser  | Tuer une cellule                        |
| Espace                | Pause / reprise                         |
| N                     | Avancer d'une generation (en pause)     |
| R                     | Grille aleatoire                        |
| C                     | Effacer la grille                       |
| + / -                 | Plus rapide / plus lent                 |
| Echap ou Q            | Quitter                                 |

## Les regles

- Une cellule vivante avec 2 ou 3 voisines vivantes survit.
- Une cellule morte avec exactement 3 voisines vivantes nait.
- Dans tous les autres cas, la cellule est morte a la generation suivante.

La grille se comporte comme un tore : le bord droit rejoint le bord gauche,
et le bord haut rejoint le bord bas.
