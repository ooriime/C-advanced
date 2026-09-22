/*
 * Jeu de la vie (Conway's Game of Life) - version simple avec SDL2
 *
 * Commandes :
 *   Clic gauche / glisser : donner la vie a une cellule
 *   Clic droit  / glisser : tuer une cellule
 *   Espace                : pause / reprise
 *   N                     : avancer d'une seule generation (en pause)
 *   R                     : grille aleatoire
 *   C                     : effacer la grille
 *   + / -                 : plus rapide / plus lent
 *   Echap ou Q            : quitter
 */

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LARGEUR   100          /* nombre de colonnes */
#define HAUTEUR   70           /* nombre de lignes   */
#define CELLULE   10           /* taille d'une cellule en pixels */

#define FENETRE_L (LARGEUR * CELLULE)
#define FENETRE_H (HAUTEUR * CELLULE)

/* Les deux grilles : la courante et celle de la generation suivante */
static int grille[HAUTEUR][LARGEUR];
static int suivante[HAUTEUR][LARGEUR];

/* Compte les voisins vivants autour de la cellule (x, y).
   La grille est un tore : les bords se rejoignent. */
static int compter_voisins(int x, int y)
{
    int total = 0;

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0)
                continue;                       /* on ne se compte pas soi-meme */

            int vx = (x + dx + LARGEUR) % LARGEUR;
            int vy = (y + dy + HAUTEUR) % HAUTEUR;

            total += grille[vy][vx];
        }
    }
    return total;
}

/* Calcule une generation avec les regles de Conway */
static void generation_suivante(void)
{
    for (int y = 0; y < HAUTEUR; y++) {
        for (int x = 0; x < LARGEUR; x++) {
            int voisins = compter_voisins(x, y);

            if (grille[y][x]) {
                /* Une cellule vivante survit avec 2 ou 3 voisins */
                suivante[y][x] = (voisins == 2 || voisins == 3);
            } else {
                /* Une cellule morte nait avec exactement 3 voisins */
                suivante[y][x] = (voisins == 3);
            }
        }
    }

    /* On recopie la nouvelle generation dans la grille courante */
    for (int y = 0; y < HAUTEUR; y++)
        for (int x = 0; x < LARGEUR; x++)
            grille[y][x] = suivante[y][x];
}

static void vider_grille(void)
{
    for (int y = 0; y < HAUTEUR; y++)
        for (int x = 0; x < LARGEUR; x++)
            grille[y][x] = 0;
}

static void grille_aleatoire(void)
{
    for (int y = 0; y < HAUTEUR; y++)
        for (int x = 0; x < LARGEUR; x++)
            grille[y][x] = (rand() % 100) < 25;   /* environ 25 % de vivantes */
}

/* Dessine les cellules vivantes puis le quadrillage */
static void dessiner(SDL_Renderer *rendu)
{
    SDL_SetRenderDrawColor(rendu, 15, 15, 20, 255);
    SDL_RenderClear(rendu);

    SDL_SetRenderDrawColor(rendu, 80, 220, 120, 255);
    for (int y = 0; y < HAUTEUR; y++) {
        for (int x = 0; x < LARGEUR; x++) {
            if (grille[y][x]) {
                SDL_Rect r = { x * CELLULE, y * CELLULE, CELLULE, CELLULE };
                SDL_RenderFillRect(rendu, &r);
            }
        }
    }

    SDL_SetRenderDrawColor(rendu, 40, 40, 50, 255);
    for (int x = 0; x <= LARGEUR; x++)
        SDL_RenderDrawLine(rendu, x * CELLULE, 0, x * CELLULE, FENETRE_H);
    for (int y = 0; y <= HAUTEUR; y++)
        SDL_RenderDrawLine(rendu, 0, y * CELLULE, FENETRE_L, y * CELLULE);

    SDL_RenderPresent(rendu);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Erreur SDL_Init : %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *fenetre = SDL_CreateWindow(
        "Jeu de la vie",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        FENETRE_L, FENETRE_H, 0);

    if (!fenetre) {
        fprintf(stderr, "Erreur fenetre : %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *rendu = SDL_CreateRenderer(fenetre, -1, SDL_RENDERER_ACCELERATED);
    if (!rendu) {
        fprintf(stderr, "Erreur rendu : %s\n", SDL_GetError());
        SDL_DestroyWindow(fenetre);
        SDL_Quit();
        return 1;
    }

    srand((unsigned)time(NULL));
    grille_aleatoire();

    int en_marche = 1;      /* boucle principale */
    int en_pause = 0;       /* simulation figee ou non */
    int delai = 100;        /* millisecondes entre deux generations */
    Uint32 dernier_tour = SDL_GetTicks();

    while (en_marche) {
        SDL_Event e;

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                en_marche = 0;
            }
            else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                case SDLK_q:      en_marche = 0;              break;
                case SDLK_SPACE:  en_pause = !en_pause;       break;
                case SDLK_n:
                    if (en_pause)
                        generation_suivante();
                    break;
                case SDLK_r:      grille_aleatoire();         break;
                case SDLK_c:      vider_grille();             break;
                case SDLK_PLUS:
                case SDLK_EQUALS:
                case SDLK_KP_PLUS:
                    delai -= 20;
                    if (delai < 10) delai = 10;
                    break;
                case SDLK_MINUS:
                case SDLK_KP_MINUS:
                    delai += 20;
                    if (delai > 1000) delai = 1000;
                    break;
                }
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEMOTION) {
                /* On dessine avec la souris quand un bouton est enfonce */
                int bx, by;
                Uint32 boutons = SDL_GetMouseState(&bx, &by);

                int x = bx / CELLULE;
                int y = by / CELLULE;

                if (x >= 0 && x < LARGEUR && y >= 0 && y < HAUTEUR) {
                    if (boutons & SDL_BUTTON(SDL_BUTTON_LEFT))
                        grille[y][x] = 1;
                    else if (boutons & SDL_BUTTON(SDL_BUTTON_RIGHT))
                        grille[y][x] = 0;
                }
            }
        }

        /* Avancer d'une generation quand le delai est ecoule */
        if (!en_pause && SDL_GetTicks() - dernier_tour >= (Uint32)delai) {
            generation_suivante();
            dernier_tour = SDL_GetTicks();
        }

        dessiner(rendu);
        SDL_Delay(10);          /* on menage le processeur */
    }

    SDL_DestroyRenderer(rendu);
    SDL_DestroyWindow(fenetre);
    SDL_Quit();
    return 0;
}
