/*
 * Jeu de la vie : la guerre des deux peuples - raylib
 *
 * Les Rouges et les Bleus vivent selon les regles de Conway :
 *   - un humain entoure de 2 ou 3 voisins (de n'importe quel peuple) survit ;
 *   - seul ou etouffe par la foule, il meurt ;
 *   - une case vide entouree d'exactement 3 humains voit naitre un bebe,
 *     qui rejoint le peuple majoritaire parmi ses 3 parents.
 * Les frontieres bougent, les planeurs d'un peuple percent les lignes de
 * l'autre... jusqu'a ce qu'un seul peuple domine le monde.
 *
 * Commandes :
 *   Clic gauche sur une case vide : creer un humain du peuple choisi (glisser)
 *   Clic gauche sur un humain     : le foudroyer (glisser)
 *   1 / 2 ou Tab                  : choisir le peuple a creer (Rouges / Bleus)
 *   Clic droit / glisser          : deplacer la vue
 *   Molette                       : zoom
 *   Espace                        : pause / reprise
 *   N                             : avancer d'une generation (en pause)
 *   R                             : deux territoires face a face
 *   M                             : les deux peuples melanges au hasard
 *   C                             : vider le monde
 *   G                             : afficher / cacher la grille
 *   F                             : recentrer la vue
 *   + / -                         : plus rapide / plus lent
 *   Echap ou Q                    : quitter
 */

#include "raylib.h"
#include "rlgl.h"
#include <math.h>
#include <string.h>
#include <time.h>

#define COLONNES   50
#define LIGNES     30
#define CASE       32.0f                 /* taille d'une case dans le monde */
#define MONDE_L    (COLONNES * CASE)
#define MONDE_H    (LIGNES * CASE)
#define MAX_EFFETS 4096
#define HISTOIRE   200                   /* generations gardees pour la courbe */
#define AGE_ADULTE 4                     /* en dessous, c'est un enfant */

typedef struct {
    int     vivant;
    int     age;             /* nombre de generations vecues */
    int     prenom;
    int     peuple;          /* 0 = Rouges, 1 = Bleus */
    Color   peau, cheveux, habit;
    float   angle;           /* direction du regard, en degres (0 = droite) */
    float   angle_cible;
    Vector2 pos, cible;      /* decalage dans la case, en fraction de case */
    float   pause;           /* temps d'arret restant avant de repartir */
    float   pas;             /* phase de la marche, pour balancer les bras */
    float   ne_a;            /* instant de la naissance (animation) */
} Humain;

typedef enum { FOUDRE, APPARITION } TypeEffet;

typedef struct {
    TypeEffet type;
    Vector2   pos;           /* dans le monde */
    float     debut, duree;
    unsigned  graine;        /* pour que l'eclair garde la meme forme */
} Effet;

static Humain grille[LIGNES][COLONNES];
static Humain suivante[LIGNES][COLONNES];
static Color  sol[LIGNES][COLONNES];
static Effet  effets[MAX_EFFETS];
static int    prochain_effet = 0;

static float maintenant = 0.0f;
static float flash = 0.0f;              /* eclat blanc quand la foudre tombe */
static int   generation = 0, foudroyes = 0;
static int   histoire[HISTOIRE][2];     /* population de chaque peuple, generation par generation */
static int   nb_histoire = 0;

static const char *prenoms[] = {
    "Adam", "Eve", "Lea", "Hugo", "Emma", "Louis", "Jade", "Gabriel", "Alice",
    "Arthur", "Lina", "Jules", "Rose", "Leo", "Mila", "Noah", "Zoe", "Tom",
    "Ines", "Paul", "Anna", "Sacha", "Lou", "Nina", "Victor", "Julia", "Oscar",
    "Iris", "Max", "Clara", "Theo", "Eden", "Remi", "Maya", "Axel", "Lucie",
    "Yanis", "Sarah", "Ethan", "Manon"
};
#define NB_PRENOMS (int)(sizeof prenoms / sizeof prenoms[0])

static const Color peaux[] = {
    {255, 224, 196, 255}, {241, 194, 155, 255}, {224, 172, 125, 255},
    {198, 134,  90, 255}, {141,  85,  54, 255}, { 98,  60,  38, 255}
};
static const Color cheveux[] = {
    { 30,  25,  22, 255}, { 70,  45,  30, 255}, {110,  70,  40, 255},
    {220, 185, 110, 255}, {170,  75,  35, 255}, { 50,  40,  35, 255}
};
static const char *noms_peuples[2] = { "Rouges", "Bleus" };
static const Color  couleurs_peuples[2] = { { 220, 60, 55, 255 }, { 55, 110, 225, 255 } };
static const Color  sols_peuples[2]     = { { 240, 180, 170, 255 }, { 170, 198, 245, 255 } };
#define NB(t) (int)(sizeof t / sizeof t[0])

/* ------------------------------------------------------------------ */
/* Petits outils                                                       */
/* ------------------------------------------------------------------ */

static float alea(float a, float b)
{
    return a + (b - a) * (float)GetRandomValue(0, 10000) / 10000.0f;
}

/* generateur pseudo-aleatoire local, pour redessiner un eclair a l'identique */
static float lcg(unsigned *s)
{
    *s = *s * 1664525u + 1013904223u;
    return (float)((*s >> 8) & 0xFFFF) / 65535.0f;
}

static Color melange(Color a, Color b, float t)
{
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}

static Color assombrir(Color c, float f)
{
    return (Color){ (unsigned char)(c.r * f), (unsigned char)(c.g * f), (unsigned char)(c.b * f), c.a };
}

static Color transparence(Color c, float a)
{
    if (a < 0) a = 0;
    if (a > 1) a = 1;
    c.a = (unsigned char)(c.a * a);
    return c;
}

/* centre d'une case dans le monde */
static Vector2 centre_case(int x, int y)
{
    return (Vector2){ (x + 0.5f) * CASE, (y + 0.5f) * CASE };
}

/* position reelle d'un humain (il se balade un peu dans sa case) */
static Vector2 position_humain(const Humain *h, int x, int y)
{
    Vector2 c = centre_case(x, y);
    return (Vector2){ c.x + h->pos.x * CASE, c.y + h->pos.y * CASE };
}

static void ajouter_effet(TypeEffet type, Vector2 pos, float duree)
{
    Effet *e = &effets[prochain_effet];
    prochain_effet = (prochain_effet + 1) % MAX_EFFETS;
    e->type   = type;
    e->pos    = pos;
    e->debut  = maintenant;
    e->duree  = duree;
    e->graine = (unsigned)GetRandomValue(1, 1 << 30);
}

/* ------------------------------------------------------------------ */
/* Les humains                                                         */
/* ------------------------------------------------------------------ */

/* Fait naitre un humain. Sans parents, ses traits sont tires au hasard. */
static void creer_humain(Humain *h, Humain *const parents[], int nb_parents, int age, int peuple)
{
    memset(h, 0, sizeof *h);
    h->vivant = 1;
    h->age    = age;
    h->prenom = GetRandomValue(0, NB_PRENOMS - 1);
    h->peuple = peuple;

    if (nb_parents > 0) {
        h->peau    = parents[GetRandomValue(0, nb_parents - 1)]->peau;
        h->cheveux = parents[GetRandomValue(0, nb_parents - 1)]->cheveux;
    } else {
        h->peau    = peaux[GetRandomValue(0, NB(peaux) - 1)];
        h->cheveux = cheveux[GetRandomValue(0, NB(cheveux) - 1)];
    }
    h->habit       = assombrir(couleurs_peuples[peuple], alea(0.75f, 1.0f));
    h->angle       = alea(0, 360);
    h->angle_cible = h->angle;
    h->pause       = alea(0, 2);
    h->ne_a        = maintenant;
}

/* Entre deux generations, chaque humain reste au centre de sa case
   (pour que les motifs restent nets) et se contente de regarder autour de lui. */
static void animer_humain(Humain *h, float dt)
{
    h->pause -= dt;
    if (h->pause <= 0) {
        h->angle_cible = alea(0, 360);
        h->pause = alea(0.8f, 3.0f);
    }

    /* tourner doucement vers la direction voulue, par le chemin le plus court */
    float diff = fmodf(h->angle_cible - h->angle + 540.0f, 360.0f) - 180.0f;
    h->angle += diff * fminf(1.0f, dt * 6.0f);
}

/* Dessine un humain vu du dessus : ombre, bras, epaules, tete, cheveux, nez. */
static void dessiner_humain(const Humain *h, Vector2 c)
{
    float croissance = (maintenant - h->ne_a) / 0.4f;     /* il "pop" a sa naissance */
    if (croissance > 1) croissance = 1;
    croissance = 1 - (1 - croissance) * (1 - croissance);

    float taille = CASE * (h->age < AGE_ADULTE ? 0.85f + 0.04f * h->age : 1.0f) * croissance;
    if (taille < 1.0f)
        return;

    /* les cheveux blanchissent avec l'age */
    Color tete_cheveux = melange(h->cheveux, (Color){ 215, 215, 215, 255 }, (h->age - 40) / 30.0f);
    float bras = (h->pause <= 0) ? sinf(h->pas) * 0.10f * taille : 0.0f;

    /* ombre (ne tourne pas avec le corps : le soleil reste au meme endroit) */
    rlPushMatrix();
    rlTranslatef(c.x + 0.06f * taille, c.y + 0.08f * taille, 0);
    DrawEllipse(0, 0, 0.30f * taille, 0.24f * taille, (Color){ 0, 0, 0, 60 });
    rlPopMatrix();

    rlPushMatrix();
    rlTranslatef(c.x, c.y, 0);
    rlRotatef(h->angle, 0, 0, 1);          /* dans ce repere, l'avant est vers +x */

    /* bras qui se balancent : manches puis mains */
    DrawCircleV((Vector2){  bras, -0.27f * taille }, 0.085f * taille, assombrir(h->habit, 0.85f));
    DrawCircleV((Vector2){ -bras,  0.27f * taille }, 0.085f * taille, assombrir(h->habit, 0.85f));
    DrawCircleV((Vector2){  bras + 0.07f * taille, -0.27f * taille }, 0.055f * taille, h->peau);
    DrawCircleV((Vector2){ -bras + 0.07f * taille,  0.27f * taille }, 0.055f * taille, h->peau);

    /* epaules */
    DrawEllipse(0, 0, 0.15f * taille, 0.29f * taille, h->habit);

    /* tete : un croissant de visage devant, les cheveux sur le dessus */
    DrawCircleV((Vector2){ 0.03f * taille, 0 }, 0.16f * taille, h->peau);
    DrawCircleSector((Vector2){ 0.03f * taille, 0 }, 0.165f * taille, 95, 265, 16, tete_cheveux);
    DrawCircleV((Vector2){ 0.0f, 0 }, 0.14f * taille, tete_cheveux);
    DrawCircleV((Vector2){ 0.19f * taille, 0 }, 0.035f * taille, assombrir(h->peau, 0.85f));

    rlPopMatrix();
}

/* ------------------------------------------------------------------ */
/* Les regles du jeu de la vie                                         */
/* ------------------------------------------------------------------ */

static void compter(int pop[2])
{
    pop[0] = pop[1] = 0;
    for (int y = 0; y < LIGNES; y++)
        for (int x = 0; x < COLONNES; x++)
            if (grille[y][x].vivant)
                pop[grille[y][x].peuple]++;
}

static void noter_histoire(void)
{
    if (nb_histoire == HISTOIRE) {                 /* on oublie la plus ancienne */
        memmove(histoire[0], histoire[1], sizeof histoire[0] * (HISTOIRE - 1));
        nb_histoire--;
    }
    compter(histoire[nb_histoire++]);
}

static void generation_suivante(void)
{
    for (int y = 0; y < LIGNES; y++) {
        for (int x = 0; x < COLONNES; x++) {
            Humain *parents[8];
            int voisins = 0;

            /* le monde est un tore : les bords se rejoignent */
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0)
                        continue;
                    Humain *v = &grille[(y + dy + LIGNES) % LIGNES][(x + dx + COLONNES) % COLONNES];
                    if (v->vivant)
                        parents[voisins++] = v;
                }

            Humain *h = &grille[y][x];
            Humain *s = &suivante[y][x];

            if (h->vivant) {
                if (voisins == 2 || voisins == 3) {
                    *s = *h;
                    s->age++;
                } else {
                    s->vivant = 0;
                }
            } else if (voisins == 3) {
                /* le bebe rejoint le peuple majoritaire parmi ses 3 parents */
                int bleus = 0;
                for (int i = 0; i < 3; i++)
                    bleus += parents[i]->peuple;
                creer_humain(s, parents, voisins, 0, bleus >= 2);
            } else {
                s->vivant = 0;
            }
        }
    }

    memcpy(grille, suivante, sizeof grille);
    generation++;
    noter_histoire();
}

static void vider_monde(void)
{
    memset(grille, 0, sizeof grille);
    generation = foudroyes = nb_histoire = 0;
}

static void peupler(int x, int y, int peuple)
{
    creer_humain(&grille[y][x], NULL, 0, GetRandomValue(AGE_ADULTE, 40), peuple);
    grille[y][x].ne_a -= alea(0, 0.3f);                  /* apparitions decalees */
}

/* Deux territoires face a face : les Rouges a gauche, les Bleus a droite,
   separes par un no man's land (le monde est un tore : ils se touchent aussi
   par les bords, mais la aussi une bande vide les separe). */
static void monde_territoires(void)
{
    vider_monde();
    for (int y = 0; y < LIGNES; y++)
        for (int x = 0; x < COLONNES; x++) {
            int bord = x < 2 || x >= COLONNES - 2 || (x >= COLONNES / 2 - 2 && x < COLONNES / 2 + 2);
            if (!bord && GetRandomValue(0, 99) < 30)
                peupler(x, y, x >= COLONNES / 2);
        }
    noter_histoire();
}

static void monde_melange(void)
{
    vider_monde();
    for (int y = 0; y < LIGNES; y++)
        for (int x = 0; x < COLONNES; x++)
            if (GetRandomValue(0, 99) < 25)
                peupler(x, y, GetRandomValue(0, 1));
    noter_histoire();
}

/* ------------------------------------------------------------------ */
/* Pouvoirs divins                                                     */
/* ------------------------------------------------------------------ */

static void creer_par_dieu(int x, int y, int peuple)
{
    creer_humain(&grille[y][x], NULL, 0, AGE_ADULTE, peuple);
    ajouter_effet(APPARITION, centre_case(x, y), 0.7f);
}

static void foudroyer(int x, int y)
{
    ajouter_effet(FOUDRE, position_humain(&grille[y][x], x, y), 1.2f);
    grille[y][x].vivant = 0;
    foudroyes++;
    flash = 1.0f;
}

/* ------------------------------------------------------------------ */
/* Dessin                                                              */
/* ------------------------------------------------------------------ */

static void preparer_sol(void)
{
    for (int y = 0; y < LIGNES; y++)
        for (int x = 0; x < COLONNES; x++)
            sol[y][x] = ((x + y) % 2) ? (Color){ 84, 140, 60, 255 } : (Color){ 90, 148, 64, 255 };
}

static void dessiner_sol(int x0, int y0, int x1, int y1, int quadrillage)
{
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            DrawRectangleRec((Rectangle){ x * CASE, y * CASE, CASE + 0.5f, CASE + 0.5f }, sol[y][x]);
            /* une case habitee s'eclaire : c'est elle qui dessine les motifs */
            if (grille[y][x].vivant)
                DrawRectangleRec((Rectangle){ x * CASE + 1, y * CASE + 1, CASE - 2, CASE - 2 }, sols_peuples[grille[y][x].peuple]);
        }

    if (quadrillage) {
        Color trait = { 0, 0, 0, 45 };
        for (int x = x0; x <= x1 + 1; x++)
            DrawLineV((Vector2){ x * CASE, y0 * CASE }, (Vector2){ x * CASE, (y1 + 1) * CASE }, trait);
        for (int y = y0; y <= y1 + 1; y++)
            DrawLineV((Vector2){ x0 * CASE, y * CASE }, (Vector2){ (x1 + 1) * CASE, y * CASE }, trait);
    }
}

/* Effets poses sur le sol (sous les humains) */
static void dessiner_effets_sol(void)
{
    for (int i = 0; i < MAX_EFFETS; i++) {
        Effet *e = &effets[i];
        float t = maintenant - e->debut;
        if (e->duree <= 0 || t < 0 || t > e->duree)
            continue;
        float k = t / e->duree;

        if (e->type == FOUDRE) {                     /* trace de brulure */
            unsigned g = e->graine;
            Color brule = transparence((Color){ 35, 28, 20, 200 }, 1 - k);
            DrawCircleV(e->pos, CASE * 0.32f, brule);
            for (int r = 0; r < 7; r++) {
                float a = lcg(&g) * 2 * PI;
                float l = CASE * (0.35f + lcg(&g) * 0.3f);
                DrawLineEx(e->pos, (Vector2){ e->pos.x + cosf(a) * l, e->pos.y + sinf(a) * l }, 2.0f, brule);
            }
        } else if (e->type == APPARITION) {          /* cercle dore qui s'ouvre */
            Color or = transparence((Color){ 255, 215, 90, 255 }, 1 - k);
            float r = CASE * (0.2f + k * 0.6f);
            DrawRing(e->pos, r - 2, r, 0, 360, 32, or);
            for (int s = 0; s < 8; s++) {
                float a = s * 45.0f * DEG2RAD + k;
                DrawCircleV((Vector2){ e->pos.x + cosf(a) * r * 1.2f, e->pos.y + sinf(a) * r * 1.2f }, 2.0f, or);
            }
        }
    }
}

/* Effets dans les airs (au-dessus des humains) */
static void dessiner_effets_air(void)
{
    for (int i = 0; i < MAX_EFFETS; i++) {
        Effet *e = &effets[i];
        float t = maintenant - e->debut;
        if (e->duree <= 0 || t < 0 || t > e->duree)
            continue;
        if (e->type == FOUDRE && t < 0.35f) { /* l'eclair qui tombe du ciel */
            if (fmodf(t, 0.08f) > 0.06f)
                continue;                            /* scintillement */
            unsigned g = e->graine;
            Vector2 a = { e->pos.x + (lcg(&g) - 0.5f) * 80, e->pos.y - 600 };
            for (int s = 1; s <= 14; s++) {
                float f = s / 14.0f;
                Vector2 b = { e->pos.x + (lcg(&g) - 0.5f) * 50 * (1 - f), a.y + (e->pos.y - a.y) / (15 - s) };
                if (s == 14) b = e->pos;
                DrawLineEx(a, b, 9.0f, (Color){ 180, 200, 255, 90 });
                DrawLineEx(a, b, 3.0f, (Color){ 255, 255, 230, 255 });
                a = b;
            }
            DrawCircleV(e->pos, CASE * 0.7f * (1 - t / 0.35f), (Color){ 255, 250, 200, 160 });
        }
    }
}

/* ------------------------------------------------------------------ */
/* Programme principal                                                 */
/* ------------------------------------------------------------------ */

static void recentrer(Camera2D *cam)
{
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    cam->offset = (Vector2){ sw / 2, sh / 2 };
    cam->target = (Vector2){ MONDE_L / 2, MONDE_H / 2 };
    cam->zoom   = fminf(sw / MONDE_L, (sh - 40) / MONDE_H) * 0.97f;
    cam->rotation = 0;
}

int main(void)
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 820, "Jeu de la vie - la guerre des peuples");
    SetTargetFPS(60);
    SetRandomSeed((unsigned int)time(NULL));

    preparer_sol();
    monde_territoires();

    Camera2D cam;
    recentrer(&cam);

    int en_pause = 0, quadrillage = 1;
    int peuple_choisi = 0;              /* le peuple que cree le clic */
    int delai = 400;                    /* millisecondes entre deux generations */
    double dernier_tour = GetTime();

    enum { RIEN, CREER, TUER } pouvoir = RIEN;   /* ce que fait le clic en cours */
    int derniere_x = -1, derniere_y = -1;

    while (!WindowShouldClose() && !IsKeyPressed(KEY_Q)) {
        float dt = GetFrameTime();
        maintenant = (float)GetTime();

        /* ----- clavier ----- */
        if (IsKeyPressed(KEY_SPACE)) en_pause = !en_pause;
        if (IsKeyPressed(KEY_R))     monde_territoires();
        if (IsKeyPressed(KEY_M))     monde_melange();
        if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) peuple_choisi = 0;
        if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) peuple_choisi = 1;
        if (IsKeyPressed(KEY_TAB))   peuple_choisi = !peuple_choisi;
        if (IsKeyPressed(KEY_C))     vider_monde();
        if (IsKeyPressed(KEY_G))     quadrillage = !quadrillage;
        if (IsKeyPressed(KEY_F))     recentrer(&cam);
        if (IsKeyPressed(KEY_N) && en_pause) generation_suivante();

        if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_UP)) {
            delai = delai * 2 / 3;
            if (delai < 30) delai = 30;
        }
        if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_DOWN)) {
            delai = delai * 3 / 2;
            if (delai > 3000) delai = 3000;
        }

        /* ----- camera : clic droit pour se deplacer, molette pour zoomer ----- */
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            Vector2 d = GetMouseDelta();
            cam.target.x -= d.x / cam.zoom;
            cam.target.y -= d.y / cam.zoom;
        }
        float molette = GetMouseWheelMove();
        if (molette != 0) {
            /* on zoome vers le point sous la souris */
            cam.target = GetScreenToWorld2D(GetMousePosition(), cam);
            cam.offset = GetMousePosition();
            cam.zoom *= (molette > 0) ? 1.15f : 1 / 1.15f;
            if (cam.zoom < 0.2f) cam.zoom = 0.2f;
            if (cam.zoom > 5.0f) cam.zoom = 5.0f;
        }

        /* ----- clic gauche : creer ou foudroyer ----- */
        Vector2 souris = GetScreenToWorld2D(GetMousePosition(), cam);
        int mx = (int)floorf(souris.x / CASE);
        int my = (int)floorf(souris.y / CASE);
        int dans_monde = (mx >= 0 && mx < COLONNES && my >= 0 && my < LIGNES);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && dans_monde) {
            /* le premier clic decide du pouvoir pour tout le glisser */
            pouvoir = grille[my][mx].vivant ? TUER : CREER;
            derniere_x = derniere_y = -1;
        }
        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            pouvoir = RIEN;

        if (pouvoir != RIEN && dans_monde && (mx != derniere_x || my != derniere_y)) {
            if (pouvoir == CREER && !grille[my][mx].vivant)
                creer_par_dieu(mx, my, peuple_choisi);
            else if (pouvoir == TUER && grille[my][mx].vivant)
                foudroyer(mx, my);
            derniere_x = mx;
            derniere_y = my;
        }

        /* ----- la vie continue ----- */
        for (int y = 0; y < LIGNES; y++)
            for (int x = 0; x < COLONNES; x++)
                if (grille[y][x].vivant)
                    animer_humain(&grille[y][x], dt);

        if (!en_pause && (GetTime() - dernier_tour) * 1000.0 >= (double)delai) {
            generation_suivante();
            dernier_tour = GetTime();
        }

        flash -= dt * 4;
        if (flash < 0) flash = 0;

        /* ----- dessin ----- */
        BeginDrawing();
        ClearBackground((Color){ 32, 70, 110, 255 });      /* l'ocean autour du monde */

        BeginMode2D(cam);

        /* on ne dessine que les cases visibles a l'ecran */
        Vector2 hg = GetScreenToWorld2D((Vector2){ 0, 0 }, cam);
        Vector2 bd = GetScreenToWorld2D((Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() }, cam);
        int x0 = (int)(hg.x / CASE) - 1, y0 = (int)(hg.y / CASE) - 1;
        int x1 = (int)(bd.x / CASE) + 1, y1 = (int)(bd.y / CASE) + 1;
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x1 > COLONNES - 1) x1 = COLONNES - 1;
        if (y1 > LIGNES - 1)   y1 = LIGNES - 1;

        DrawRectangleRec((Rectangle){ -6, -6, MONDE_L + 12, MONDE_H + 12 }, (Color){ 214, 196, 140, 255 }); /* plage */
        dessiner_sol(x0, y0, x1, y1, quadrillage);
        dessiner_effets_sol();

        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++)
                if (grille[y][x].vivant)
                    dessiner_humain(&grille[y][x], position_humain(&grille[y][x], x, y));

        dessiner_effets_air();

        /* la case sous la main de dieu */
        if (dans_monde) {
            if (grille[my][mx].vivant) {
                float r = CASE * 0.45f;
                DrawRing(position_humain(&grille[my][mx], mx, my), r, r + 2.5f, 0, 360, 32, (Color){ 255, 70, 60, 220 });
            } else {
                DrawRectangleLinesEx((Rectangle){ mx * CASE + 2, my * CASE + 2, CASE - 4, CASE - 4 }, 2.5f, couleurs_peuples[peuple_choisi]);
            }
        }

        EndMode2D();

        if (flash > 0)
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), transparence(WHITE, flash * 0.35f));

        /* ----- panneau d'informations ----- */
        int pop[2];
        compter(pop);
        int total = pop[0] + pop[1];

        DrawRectangleRounded((Rectangle){ 10, 10, 260, 236 }, 0.08f, 8, (Color){ 10, 15, 25, 200 });
        DrawText(TextFormat("Generation %d", generation), 22, 20, 20, RAYWHITE);
        for (int p = 0; p < 2; p++) {
            DrawRectangle(22, 52 + p * 22, 12, 12, couleurs_peuples[p]);
            DrawText(TextFormat("%s : %d", noms_peuples[p], pop[p]), 42, 50 + p * 22, 16, RAYWHITE);
        }

        /* rapport de forces */
        float part_rouge = total > 0 ? (float)pop[0] / total : 0.5f;
        DrawRectangle(22, 98, 236, 10, couleurs_peuples[1]);
        DrawRectangle(22, 98, (int)(236 * part_rouge), 10, couleurs_peuples[0]);
        DrawRectangle(22 + 118, 96, 1, 14, RAYWHITE);

        /* courbe des populations sur les dernieres generations */
        Rectangle g = { 22, 118, 236, 60 };
        DrawRectangleRec(g, (Color){ 25, 30, 45, 255 });
        int maxi = 1;
        for (int i = 0; i < nb_histoire; i++)
            for (int p = 0; p < 2; p++)
                if (histoire[i][p] > maxi) maxi = histoire[i][p];
        for (int p = 0; p < 2; p++)
            for (int i = 1; i < nb_histoire; i++) {
                Vector2 a = { g.x + g.width * (i - 1) / (HISTOIRE - 1), g.y + g.height * (1 - (float)histoire[i - 1][p] / maxi) };
                Vector2 b = { g.x + g.width * i / (HISTOIRE - 1), g.y + g.height * (1 - (float)histoire[i][p] / maxi) };
                DrawLineEx(a, b, 2, couleurs_peuples[p]);
            }

        DrawText("Tu crees des :", 22, 190, 16, LIGHTGRAY);
        DrawRectangle(142, 191, 12, 12, couleurs_peuples[peuple_choisi]);
        DrawText(noms_peuples[peuple_choisi], 160, 190, 16, couleurs_peuples[peuple_choisi]);
        if (en_pause)
            DrawText("PAUSE", 22, 214, 16, (Color){ 255, 120, 110, 255 });
        else
            DrawText(TextFormat("%d ms / generation", delai), 22, 214, 16, GRAY);

        /* un seul peuple survit : il a gagne */
        if (generation > 0 && total > 0 && (pop[0] == 0 || pop[1] == 0)) {
            int gagnant = pop[0] > 0 ? 0 : 1;
            const char *txt = TextFormat("Les %s ont conquis le monde !", noms_peuples[gagnant]);
            int l = MeasureText(txt, 40);
            int x = (GetScreenWidth() - l) / 2, y = GetScreenHeight() / 2 - 30;
            DrawRectangleRounded((Rectangle){ (float)x - 24, (float)y - 16, (float)l + 48, 72 }, 0.3f, 8, (Color){ 10, 15, 25, 220 });
            DrawText(txt, x, y, 40, couleurs_peuples[gagnant]);
        }

        /* nom de l'humain survole */
        if (dans_monde && grille[my][mx].vivant) {
            const char *info = TextFormat("%s des %s, %d ans", prenoms[grille[my][mx].prenom],
                                          noms_peuples[grille[my][mx].peuple], grille[my][mx].age);
            int l = MeasureText(info, 18);
            Vector2 m = GetMousePosition();
            DrawRectangleRounded((Rectangle){ m.x + 14, m.y - 30, (float)l + 16, 26 }, 0.4f, 6, (Color){ 10, 15, 25, 210 });
            DrawText(info, (int)m.x + 22, (int)m.y - 26, 18, RAYWHITE);
        }

        const char *aide = "Clic gauche : creer / foudroyer   1/2/Tab : peuple   Clic droit : deplacer   Molette : zoom   "
                           "Espace : pause   N : pas   R : territoires   M : melange   C : vider   G : grille   +/- : vitesse";
        DrawRectangle(0, GetScreenHeight() - 28, GetScreenWidth(), 28, (Color){ 10, 15, 25, 190 });
        DrawText(aide, 12, GetScreenHeight() - 21, 14, LIGHTGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
