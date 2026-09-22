/*
 * Jeu de la vie (Conway's Game of Life) - version simple avec raylib
 *
 * La fenetre est redimensionnable : la taille d'une cellule ne change pas,
 * c'est le NOMBRE de colonnes et de lignes qui s'adapte a la fenetre.
 * Agrandir la fenetre donne donc plus de place au jeu.
 *
 * Commandes :
 *   Clic gauche / glisser : creer ou supprimer une cellule
 *   Clic droit  / glisser : supprimer une cellule (gomme)
 *   Espace                : pause / reprise
 *   C                     : tout effacer
 *   R                     : nouvelle grille aleatoire
 *   N                     : avancer d'une seule generation (en pause)
 *   + / -                 : plus rapide / plus lent
 *   Echap ou Q            : quitter
 */

#include "raylib.h"
#include <time.h>       /* pour initialiser le hasard */

#define CELLULE     10          /* taille d'une cellule en pixels */

/* Taille maximale de la grille : de quoi couvrir un ecran 5120 x 2880.
   On reserve la place une fois pour toutes, ca evite toute allocation. */
#define MAX_LARGEUR 512
#define MAX_HAUTEUR 288

/* Taille de la fenetre au demarrage */
#define FENETRE_L   1000
#define FENETRE_H   700

/* Taille minimale de la fenetre, pour garder une grille utilisable */
#define MINI_L      200
#define MINI_H      150

/* Les deux grilles : la courante et celle de la generation suivante.
   Seule la partie [0..lignes) x [0..colonnes) est reellement utilisee. */
static int grille[MAX_HAUTEUR][MAX_LARGEUR];
static int suivante[MAX_HAUTEUR][MAX_LARGEUR];

/* Dimensions courantes de la grille, recalculees a chaque redimensionnement */
static int colonnes = 0;
static int lignes   = 0;

/* Compte les voisins vivants autour de la cellule (x, y).
   La grille est un tore : les bords se rejoignent. */
static int compter_voisins(int x, int y)
{
    int total = 0;

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0)
                continue;                       /* on ne se compte pas soi-meme */

            int vx = (x + dx + colonnes) % colonnes;
            int vy = (y + dy + lignes)   % lignes;

            total += grille[vy][vx];
        }
    }
    return total;
}

/* Calcule une generation avec les regles de Conway */
static void generation_suivante(void)
{
    for (int y = 0; y < lignes; y++) {
        for (int x = 0; x < colonnes; x++) {
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
    for (int y = 0; y < lignes; y++)
        for (int x = 0; x < colonnes; x++)
            grille[y][x] = suivante[y][x];
}

static void vider_grille(void)
{
    for (int y = 0; y < lignes; y++)
        for (int x = 0; x < colonnes; x++)
            grille[y][x] = 0;
}

static void grille_aleatoire(void)
{
    for (int y = 0; y < lignes; y++)
        for (int x = 0; x < colonnes; x++)
            grille[y][x] = (GetRandomValue(0, 99) < 25);  /* environ 25 % de vivantes */
}

/* Petit utilitaire : borne une valeur entre un minimum et un maximum */
static int borner(int valeur, int mini, int maxi)
{
    if (valeur < mini) return mini;
    if (valeur > maxi) return maxi;
    return valeur;
}

/* Recalcule le nombre de colonnes et de lignes d'apres la taille de la
   fenetre. Les cellules deja presentes sont conservees ; les cases qui
   viennent d'apparaitre (fenetre agrandie) demarrent vides. */
static void adapter_a_la_fenetre(void)
{
    int anciennes_colonnes = colonnes;
    int anciennes_lignes   = lignes;

    colonnes = borner(GetScreenWidth()  / CELLULE, 1, MAX_LARGEUR);
    lignes   = borner(GetScreenHeight() / CELLULE, 1, MAX_HAUTEUR);

    /* On vide uniquement la zone nouvellement decouverte, sinon d'anciennes
       cellules d'un agrandissement precedent reapparaitraient. */
    for (int y = 0; y < lignes; y++)
        for (int x = 0; x < colonnes; x++)
            if (x >= anciennes_colonnes || y >= anciennes_lignes)
                grille[y][x] = 0;
}

/* Applique l'action de la souris sur la cellule (x, y).
 *
 * Le clic gauche cree ou supprime une cellule : au moment ou on appuie, on
 * regarde la cellule visee ; si elle est vivante on passe en mode gomme,
 * sinon en mode crayon. Le mode est ensuite garde tant que le bouton reste
 * enfonce, ce qui permet de tracer (ou d'effacer) toute une trainee en
 * glissant, sans que les cellules clignotent a chaque image.
 *
 * Le clic droit reste une gomme pure, bien pratique pour corriger.
 *
 * mode vaut 1 pour crayon et 0 pour gomme ; il est conserve entre deux images.
 */
static void appliquer_souris(int x, int y, int debut_clic_gauche,
                             int gauche_enfonce, int droit_enfonce, int *mode)
{
    if (x < 0 || x >= colonnes || y < 0 || y >= lignes)
        return;                         /* la souris est hors de la grille */

    if (debut_clic_gauche)
        *mode = grille[y][x] ? 0 : 1;

    if (gauche_enfonce)
        grille[y][x] = *mode;

    if (droit_enfonce)
        grille[y][x] = 0;
}

/* Dessine les cellules vivantes puis le quadrillage */
static void dessiner(int en_pause, int delai)
{
    const Color fond    = { 15, 15, 20, 255 };
    const Color vivante = { 80, 220, 120, 255 };
    const Color trait   = { 40, 40, 50, 255 };

    /* La grille peut ne pas couvrir toute la fenetre si sa largeur n'est pas
       un multiple exact de CELLULE : on dessine jusqu'au bord de la grille. */
    int large = colonnes * CELLULE;
    int haut  = lignes   * CELLULE;

    BeginDrawing();
    ClearBackground(fond);

    for (int y = 0; y < lignes; y++)
        for (int x = 0; x < colonnes; x++)
            if (grille[y][x])
                DrawRectangle(x * CELLULE, y * CELLULE, CELLULE, CELLULE, vivante);

    for (int x = 0; x <= colonnes; x++)
        DrawLine(x * CELLULE, 0, x * CELLULE, haut, trait);
    for (int y = 0; y <= lignes; y++)
        DrawLine(0, y * CELLULE, large, y * CELLULE, trait);

    if (en_pause)
        DrawText("PAUSE", 10, 10, 20, RAYWHITE);
    else
        DrawText(TextFormat("%d ms / generation", delai), 10, 10, 20, GRAY);

    DrawText(TextFormat("%d x %d cellules", colonnes, lignes), 10, 34, 20, GRAY);

    EndDrawing();
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(FENETRE_L, FENETRE_H, "Jeu de la vie");
    SetWindowMinSize(MINI_L, MINI_H);
    SetTargetFPS(60);

    /* GetTime() vaut ~0 au demarrage : on prend l'heure systeme pour que
       la grille aleatoire soit differente a chaque lancement. */
    SetRandomSeed((unsigned int)time(NULL));

    adapter_a_la_fenetre();
    grille_aleatoire();

    int en_pause = 0;       /* simulation figee ou non */
    int delai = 100;        /* millisecondes entre deux generations */
    int mode_souris = 1;    /* 1 = le clic gauche cree, 0 = il supprime */
    double dernier_tour = GetTime();

    /* WindowShouldClose() est vrai si on ferme la fenetre ou si on tape Echap */
    while (!WindowShouldClose() && !IsKeyPressed(KEY_Q)) {

        /* ----- la fenetre a change de taille : on adapte la grille ----- */
        if (IsWindowResized())
            adapter_a_la_fenetre();

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

        /* ----- souris : creer ou supprimer des cellules ----- */
        appliquer_souris(GetMouseX() / CELLULE,
                         GetMouseY() / CELLULE,
                         IsMouseButtonPressed(MOUSE_BUTTON_LEFT),
                         IsMouseButtonDown(MOUSE_BUTTON_LEFT),
                         IsMouseButtonDown(MOUSE_BUTTON_RIGHT),
                         &mode_souris);

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
