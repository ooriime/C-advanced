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

| Touche                | Effet                                       |
|-----------------------|---------------------------------------------|
| Clic gauche           | Creer ou supprimer une cellule              |
| Clic gauche + glisser | Tracer ou effacer toute une trainee         |
| Clic droit            | Supprimer une cellule (gomme)               |
| Espace                | Pause / reprise                             |
| C                     | Tout effacer                                |
| R                     | Nouvelle grille aleatoire                   |
| N                     | Avancer d'une generation (en pause)         |
| + / -                 | Plus rapide / plus lent                     |
| Echap ou Q            | Quitter                                     |

Le clic gauche fonctionne comme un interrupteur : cliquer sur une case vide
cree une cellule, cliquer sur une cellule vivante la supprime. En gardant le
bouton enfonce, le mode choisi au moment du clic est conserve, ce qui permet
de dessiner (ou d'effacer) une ligne entiere d'un seul geste.

## Fenetre redimensionnable

La fenetre peut etre redimensionnee librement. La taille d'une cellule ne
change pas : c'est le **nombre** de colonnes et de lignes qui s'adapte.
Agrandir la fenetre donne donc plus de place au jeu, et les cellules deja
posees restent en place. Le nombre de cellules est affiche en haut a gauche.

## Les regles

- Une cellule vivante avec 2 ou 3 voisines vivantes survit.
- Une cellule morte avec exactement 3 voisines vivantes nait.
- Dans tous les autres cas, la cellule est morte a la generation suivante.

La grille se comporte comme un tore : le bord droit rejoint le bord gauche,
et le bord haut rejoint le bord bas.
