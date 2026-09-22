/*
 * Jeu de la vie (Conway's Game of Life) - version simple avec raylib
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

#include "raylib.h"
#include <time.h>       /* pour initialiser le hasard */

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
            grille[y][x] = (GetRandomValue(0, 99) < 25);  /* environ 25 % de vivantes */
}

/* Dessine les cellules vivantes puis le quadrillage */
static void dessiner(int en_pause, int delai)
{
    const Color fond    = { 15, 15, 20, 255 };
    const Color vivante = { 80, 220, 120, 255 };
    const Color trait   = { 40, 40, 50, 255 };

    BeginDrawing();
    ClearBackground(fond);

    for (int y = 0; y < HAUTEUR; y++)
        for (int x = 0; x < LARGEUR; x++)
            if (grille[y][x])
                DrawRectangle(x * CELLULE, y * CELLULE, CELLULE, CELLULE, vivante);

    for (int x = 0; x <= LARGEUR; x++)
        DrawLine(x * CELLULE, 0, x * CELLULE, FENETRE_H, trait);
    for (int y = 0; y <= HAUTEUR; y++)
        DrawLine(0, y * CELLULE, FENETRE_L, y * CELLULE, trait);

    if (en_pause)
        DrawText("PAUSE", 10, 10, 20, RAYWHITE);
    else
        DrawText(TextFormat("%d ms / generation", delai), 10, 10, 20, GRAY);

    EndDrawing();
}

int main(void)
{
    InitWindow(FENETRE_L, FENETRE_H, "Jeu de la vie");
    SetTargetFPS(60);

    /* GetTime() vaut ~0 au demarrage : on prend l'heure systeme pour que
       la grille aleatoire soit differente a chaque lancement. */
    SetRandomSeed((unsigned int)time(NULL));
    grille_aleatoire();

    int en_pause = 0;       /* simulation figee ou non */
    int delai = 100;        /* millisecondes entre deux generations */
    double dernier_tour = GetTime();

    /* WindowShouldClose() est vrai si on ferme la fenetre ou si on tape Echap */
    while (!WindowShouldClose() && !IsKeyPressed(KEY_Q)) {

        /* ----- clavier ----- */
        if (IsKeyPressed(KEY_SPACE)) en_pause = !en_pause;
        if (IsKeyPressed(KEY_R))     grille_aleatoire();
        if (IsKeyPressed(KEY_C))     vider_grille();
        if (IsKeyPressed(KEY_N) && en_pause) generation_suivante();

        if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
            delai -= 20;
            if (delai < 10) delai = 10;
        }
        if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
            delai += 20;
            if (delai > 1000) delai = 1000;
        }

        /* ----- souris : on dessine tant qu'un bouton est enfonce ----- */
        int x = GetMouseX() / CELLULE;
        int y = GetMouseY() / CELLULE;

        if (x >= 0 && x < LARGEUR && y >= 0 && y < HAUTEUR) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                grille[y][x] = 1;
            else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
                grille[y][x] = 0;
        }

        /* ----- avancer d'une generation quand le delai est ecoule ----- */
        if (!en_pause && (GetTime() - dernier_tour) * 1000.0 >= (double)delai) {
            generation_suivante();
            dernier_tour = GetTime();
        }

        dessiner(en_pause, delai);
    }

    CloseWindow();
    return 0;
}
