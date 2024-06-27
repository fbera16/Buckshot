#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

// Constants for window and button dimensions
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int BUTTON_WIDTH = 200;
const int BUTTON_HEIGHT = 50;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

const int GRID_ROWS = 2;
const int GRID_COLS = 3;
const int CELL_WIDTH = (SCREEN_WIDTH - 100) / GRID_COLS;
const int CELL_HEIGHT = SCREEN_HEIGHT / GRID_ROWS;

const int SUBGRID_ROWS = 2;
const int SUBGRID_COLS = 2;
const int SUBGRID_MARGIN = 5;
const int SUBGRID_CELL_WIDTH = (CELL_WIDTH - (SUBGRID_COLS + 1) * SUBGRID_MARGIN) / SUBGRID_COLS;
const int SUBGRID_CELL_HEIGHT = (CELL_HEIGHT - (SUBGRID_ROWS + 1) * SUBGRID_MARGIN) / SUBGRID_ROWS;

SDL_Texture *imageTexture = NULL;
SDL_Rect imageRect;

#define ROUGE 0
#define NOIR 1

// Function prototypes
bool initializeSDL();
bool loadMedia();
void closeSDL();
void renderTitle();
void renderButtons();
bool renderGame(); // Render function for the game state
bool quit = false;

// SDL variables
SDL_Window *gWindow = NULL;
SDL_Renderer *gRenderer = NULL;
SDL_Texture *gTitleTexture = NULL;
SDL_Texture *gLaunchButtonTexture = NULL;
SDL_Texture *gQuitButtonTexture = NULL;
SDL_Rect gTitleRect;                              // Rectangle for the title
SDL_Rect gLaunchButtonRect = {100, 100, 200, 50}; // Exemple de position et taille
SDL_Rect gQuitButtonRect = {100, 200, 200, 50};   // Exemple de position et taille

typedef enum
{
    CIGARETTE,
    BIERRE,
    LOUPE,
    MENOTTES,
    Null
} Object;

typedef struct
{
    SDL_Rect rect;
    bool clicked;
    int id;
    Object object;
    SDL_Texture *texture;
} GridCell;

enum GameState
{
    STATE_MENU,
    STATE_GAME,
    STATE_QUIT
};

SDL_Texture *loadTexture(SDL_Renderer *renderer, const char *path)
{
    SDL_Surface *loadedSurface = IMG_Load(path);
    if (loadedSurface == NULL)
    {
        printf("Unable to load image %s! SDL_image Error: %s\n", path, IMG_GetError());
        return NULL;
    }

    SDL_Texture *newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
    SDL_FreeSurface(loadedSurface);

    if (newTexture == NULL)
    {
        printf("Unable to create texture from %s! SDL Error: %s\n", path, SDL_GetError());
    }

    return newTexture;
}

void renderText(SDL_Renderer *renderer, const char *text, int x, int y, TTF_Font *font, SDL_Color color)
{
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect destRect = {x, y, surface->w, surface->h};
    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, NULL, &destRect);
    SDL_DestroyTexture(texture);
}

void drawGrid(SDL_Renderer *renderer, GridCell grid[GRID_ROWS][GRID_COLS], GridCell subgrids[4][SUBGRID_ROWS][SUBGRID_COLS], GridCell extraCells[2], TTF_Font *font, int nbrRouge, int nbrNoir, int nbrBalles, int manche, int vieJoueur, int vieOrdi)
{
    // Draw main grid
    // Set background color to black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer); // Clear screen with black color

    // Draw main grid
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Couleur blanche pour les bordures
    for (int i = 0; i < GRID_ROWS; i++)
    {
        for (int j = 0; j < GRID_COLS; j++)
        {
            SDL_RenderDrawRect(renderer, &grid[i][j].rect);
        }
    }

    // Draw subgrids
    for (int g = 0; g < 4; g++)
    {
        for (int i = 0; i < SUBGRID_ROWS; i++)
        {
            for (int j = 0; j < SUBGRID_COLS; j++)
            {
                SDL_RenderDrawRect(renderer, &subgrids[g][i][j].rect);
                if (subgrids[g][i][j].texture != NULL)
                {
                    SDL_RenderCopy(renderer, subgrids[g][i][j].texture, NULL, &subgrids[g][i][j].rect);
                }
            }
        }
    }

    // Draw extra cells
    for (int i = 0; i < 2; i++)
    {
        SDL_RenderDrawRect(renderer, &extraCells[i].rect);
    }

    SDL_Color color = {255, 255, 255, 255}; // white
    char buffer[128];

    // Information about red and black balls
    snprintf(buffer, sizeof(buffer), "%d RED", nbrRouge);
    renderText(renderer, buffer, extraCells[0].rect.x + 10, extraCells[0].rect.y + 10, font, color);
    snprintf(buffer, sizeof(buffer), "%d BLANK", nbrNoir);
    renderText(renderer, buffer, extraCells[0].rect.x + 10, extraCells[0].rect.y + 30, font, color);
    snprintf(buffer, sizeof(buffer), "Total: %d", nbrBalles);
    renderText(renderer, buffer, extraCells[0].rect.x + 10, extraCells[0].rect.y + 50, font, color);

    // Information about the game state
    snprintf(buffer, sizeof(buffer), "Round %d", manche);
    renderText(renderer, buffer, extraCells[1].rect.x + 10, extraCells[1].rect.y + 10, font, color);
    snprintf(buffer, sizeof(buffer), "You: %d", vieJoueur);
    renderText(renderer, buffer, extraCells[1].rect.x + 10, extraCells[1].rect.y + 30, font, color);
    snprintf(buffer, sizeof(buffer), "Dealer: %d", vieOrdi);
    renderText(renderer, buffer, extraCells[1].rect.x + 10, extraCells[1].rect.y + 50, font, color);
}

void genererBalles(int *balles, int *rouge, int *noir, int *nombreDeBalles)
{
    srand(time(NULL));
    *nombreDeBalles = (rand() % 7) + 2;
    *rouge = 0;
    *noir = 0;

    for (int i = 0; i < *nombreDeBalles; i++)
    {
        balles[i] = rand() % 2;
        if (balles[i] == ROUGE)
        {
            (*rouge)++;
        }
        else
        {
            (*noir)++;
        }
    }

    if (*rouge == 0)
    {
        balles[*nombreDeBalles - 1] = ROUGE;
        (*rouge)++;
        (*noir)--;
    }

    if (*noir == 0)
    {
        balles[*nombreDeBalles - 1] = NOIR;
        (*noir)++;
        (*rouge)--;
    }

    printf("Balles générées:\n");
    for (int i = 0; i < *nombreDeBalles; i++)
    {
        printf("Balle %d: %s\n", i + 1, balles[i] == ROUGE ? "Rouge" : "Noir");
    }
}

int handleMouseClick(int x, int y, GridCell grid[GRID_ROWS][GRID_COLS], GridCell subgrids[4][SUBGRID_ROWS][SUBGRID_COLS], GridCell extraCells[2], SDL_Renderer *renderer)
{
    // Check subgrid cells first
    for (int g = 0; g < 4; ++g)
    {
        for (int i = 0; i < SUBGRID_ROWS; ++i)
        {
            for (int j = 0; j < SUBGRID_COLS; ++j)
            {
                if (SDL_PointInRect(&(SDL_Point){x, y}, &subgrids[g][i][j].rect))
                {
                    subgrids[g][i][j].clicked = true;
                    printf("Le joueur a cliqué sur la case %d dans la sous-grille %d\n", subgrids[g][i][j].id, g);
                    return subgrids[g][i][j].id; // Return subgrid ID
                }
            }
        }
    }

    // Check main grid cells
    for (int i = 0; i < GRID_ROWS; ++i)
    {
        for (int j = 0; j < GRID_COLS; ++j)
        {
            if (SDL_PointInRect(&(SDL_Point){x, y}, &grid[i][j].rect))
            {
                grid[i][j].clicked = true;
                printf("Le joueur a cliqué sur la case %d\n", grid[i][j].id);
                return grid[i][j].id;
            }
        }
    }

    // Check extra cells
    for (int i = 0; i < 2; ++i)
    {
        if (SDL_PointInRect(&(SDL_Point){x, y}, &extraCells[i].rect))
        {
            extraCells[i].clicked = true;
            printf("Le joueur a cliqué sur la case supplémentaire %d\n", extraCells[i].id);
            return extraCells[i].id;
        }
    }

    return -1; // No click detected
}

bool joueurTour(int idCase, int *vieJoueur, int *vieOrdi, int *rouges, int *noirs, int *nombreDeBalles, int *balles)
{
    printf("Le joueur a cliqué sur la case avec l'ID: %d\n", idCase);
    printf("La couleur de la balle est: %s\n", balles[0] == ROUGE ? "Rouge" : "Noir");
    if (idCase == 1)
    {
        if (balles[0] == ROUGE)
        {
            printf("Le joueur a tiré sur l'ordinateur et la balle était rouge\n");
            *vieOrdi -= 1;
            *rouges -= 1;
            *nombreDeBalles -= 1;
            for (int i = 0; i < *nombreDeBalles; i++)
            {
                balles[i] = balles[i + 1];
            }
            balles[*nombreDeBalles] = -1;
            printf("Balles restantes apres tour joueur:\n");
            for (int i = 0; i < *nombreDeBalles; i++)
            {
                printf("Balle %d: %s\n", i + 1, balles[i] == ROUGE ? "Rouge" : "Noir");
            }
        }
        else
        {
            printf("Le joueur a tiré sur l'ordinateur et la balle était noire\n");
            *noirs -= 1;
            *nombreDeBalles -= 1;
            for (int i = 0; i < *nombreDeBalles; i++)
            {
                balles[i] = balles[i + 1];
            }
            balles[*nombreDeBalles] = -1;
            printf("Balles restantes apres tour joueur:\n");
            for (int i = 0; i < *nombreDeBalles; i++)
            {
                printf("Balle %d: %s\n", i + 1, balles[i] == ROUGE ? "Rouge" : "Noir");
            }
        }
    }
    else if (idCase == 4)
    {
        if (balles[0] == ROUGE)
        {
            printf("Le joueur a tiré sur lui-même et la balle était rouge\n");
            *vieJoueur -= 1;
            *rouges -= 1;
            *nombreDeBalles -= 1;
            for (int i = 0; i < *nombreDeBalles; i++)
            {
                balles[i] = balles[i + 1];
            }
            balles[*nombreDeBalles] = -1;
            printf("Balles restantes apres tour joueur:\n");
            for (int i = 0; i < *nombreDeBalles; i++)
            {
                printf("Balle %d: %s\n", i + 1, balles[i] == ROUGE ? "Rouge" : "Noir");
            }
        }
        else
        {
            printf("Le joueur a tiré sur lui-même et la balle était noire\n");
            *noirs -= 1;
            *nombreDeBalles -= 1;
            for (int i = 0; i < *nombreDeBalles; i++)
            {
                balles[i] = balles[i + 1];
            }
            balles[*nombreDeBalles] = -1;
            printf("Balles restantes apres tour joueur:\n");
            for (int i = 0; i < *nombreDeBalles; i++)
            {
                printf("Balle %d: %s\n", i + 1, balles[i] == ROUGE ? "Rouge" : "Noir");
            }
            return true;
        }
    }

    if (*vieJoueur <= 0)
    {
        printf("Le joueur a perdu!\n");
        return true;
    }
    else if (*vieOrdi <= 0)
    {
        printf("Le joueur a gagné!\n");
        return true;
    }
    return false;
}

bool ordinateurTour(int *vieJoueur, int *vieOrdi, int *rouges, int *noirs, int *nombreDeBalles, int *balles)
{
    if (*nombreDeBalles == 0)
    {
        printf("Il n'y a plus de balles.\n");
        return false;
    }

    // L'ordinateur va essayer de maximiser son avantage
    int cible = -1;
    if (*rouges > *noirs && *rouges > 0) // Plus de balles rouges que de noires, attaquer joueur
    {
        cible = 1; // Tirer sur le joueur
    }
    else if (*noirs > *rouges && *noirs > 0) // Plus de balles noires que de rouges, tirer sur soi-même
    {
        cible = 2; // Tirer sur soi-même
    }
    else // Nombre égal de balles rouges et noires, ou une seule couleur restante
    {
        // Choisir au hasard, mais donner une légère préférence à tirer sur le joueur
        srand(time(NULL));
        cible = (rand() % 3 == 0) ? 2 : 1;
    }

    if (cible == 1)
    {
        printf("L'ordinateur a décidé de tirer sur le joueur.\n");
        if (balles[0] == ROUGE)
        {
            printf("L'ordinateur a tiré sur le joueur et la balle était rouge.\n");
            *vieJoueur -= 1;
            *rouges -= 1;
        }
        else
        {
            printf("L'ordinateur a tiré sur le joueur et la balle était noire.\n");
            *noirs -= 1;
        }
    }
    else
    {
        printf("L'ordinateur a décidé de tirer sur lui-même.\n");
        if (balles[0] == ROUGE)
        {
            printf("L'ordinateur a tiré sur lui-même et la balle était rouge.\n");
            *vieOrdi -= 1;
            *rouges -= 1;
        }
        else
        {
            printf("L'ordinateur a tiré sur lui-même et la balle était noire.\n");
            *noirs -= 1;
            return true;
        }
    }

    // Enlever la balle utilisée
    *nombreDeBalles -= 1;
    for (int i = 0; i < *nombreDeBalles; i++)
    {
        balles[i] = balles[i + 1];
    }
    balles[*nombreDeBalles] = -1;

    printf("Balles restantes après le tour de l'ordinateur:\n");
    for (int i = 0; i < *nombreDeBalles; i++)
    {
        printf("Balle %d: %s\n", i + 1, balles[i] == ROUGE ? "Rouge" : "Noir");
    }

    if (*vieJoueur <= 0)
    {
        printf("Le joueur a perdu!\n");
        return true;
    }
    else if (*vieOrdi <= 0)
    {
        printf("L'ordinateur a perdu!\n");
        return true;
    }

    return false;
}

void giveObject(GridCell subgrids[4][SUBGRID_ROWS][SUBGRID_COLS], SDL_Texture *textures[4])
{
    srand(time(NULL));
    // Prend tout les index des cases 6 a 13 et ajoute les a une liste de case vide
    int emptyCellPC[8][3] = {{0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 1, 1}, {1, 0, 0}, {1, 0, 1}, {1, 1, 0}, {1, 1, 1}};
    int emptyCellJoueur[8][3] = {{2, 0, 0}, {2, 0, 1}, {2, 1, 0}, {2, 1, 1}, {3, 0, 0}, {3, 0, 1}, {3, 1, 0}, {3, 1, 1}};
    int emptyCountPC = 8;
    int emptyCountJoueur = 8;
    // Choisir aléatoirement entre 1 et 4 le nombre d'objets
    int nombreObjets = rand() % 4 + 1;
    printf("L'ordinateur a trouvé %d objets\n", nombreObjets);
    for (int i = 0; i < nombreObjets; i++)
    {
        if (emptyCountPC == 0)
        {
            printf("Il n'y a plus de cases vides disponibles.\n");
            break;
        }
        else
        {

            int randIndex = rand() % emptyCountPC;
            int x = emptyCellPC[randIndex][0];
            int y = emptyCellPC[randIndex][1];
            int z = emptyCellPC[randIndex][2];

            // Déplacer la dernière case vide à la place de celle utilisée
            emptyCellPC[randIndex][0] = emptyCellPC[emptyCountPC - 1][0];
            emptyCellPC[randIndex][1] = emptyCellPC[emptyCountPC - 1][1];
            emptyCellPC[randIndex][2] = emptyCellPC[emptyCountPC - 1][2];
            --emptyCountPC;

            int objet = rand() % 4;

            switch (objet)
            {
            case CIGARETTE:
                subgrids[x][y][z].object = CIGARETTE;
                subgrids[x][y][z].texture = textures[CIGARETTE];
                printf("L'ordinateur a trouvé une cigarette et l'a mise dans la case %d\n", subgrids[x][y][z].id);
                break;
            case BIERRE:
                subgrids[x][y][z].object = BIERRE;
                subgrids[x][y][z].texture = textures[BIERRE];
                printf("L'ordinateur a trouvé une bière et l'a mise dans la case %d\n", subgrids[x][y][z].id);
                break;
            case LOUPE:
                subgrids[x][y][z].object = LOUPE;
                subgrids[x][y][z].texture = textures[LOUPE];
                printf("L'ordinateur a trouvé une loupe et l'a mise dans la case %d\n", subgrids[x][y][z].id);
                break;
            case MENOTTES:
                subgrids[x][y][z].object = MENOTTES;
                subgrids[x][y][z].texture = textures[MENOTTES];
                printf("L'ordinateur a trouvé des menottes et les a mises dans la case %d\n", subgrids[x][y][z].id);
                break;
            default:
                printf("mauvais objet\n");
                break;
            }
        }
    }
    for (int i = 0; i < nombreObjets; i++)
    {
        if (emptyCountJoueur == 0)
        {
            printf("Il n'y a plus de cases vides disponibles.\n");
            break;
        }
        else
        {
            int randIndex = rand() % emptyCountJoueur;
            int x = emptyCellJoueur[randIndex][0];
            int y = emptyCellJoueur[randIndex][1];
            int z = emptyCellJoueur[randIndex][2];

            // Déplacer la dernière case vide à la place de celle utilisée
            emptyCellJoueur[randIndex][0] = emptyCellJoueur[emptyCountJoueur - 1][0];
            emptyCellJoueur[randIndex][1] = emptyCellJoueur[emptyCountJoueur - 1][1];
            emptyCellJoueur[randIndex][2] = emptyCellJoueur[emptyCountJoueur - 1][2];
            --emptyCountJoueur;

            int objet = rand() % 4;

            switch (objet)
            {
            case CIGARETTE:
                subgrids[x][y][z].object = CIGARETTE;
                subgrids[x][y][z].texture = textures[CIGARETTE];
                printf("Le joueur a trouvé une cigarette et l'a mise dans la case %d\n", subgrids[x][y][z].id);
                break;
            case BIERRE:
                subgrids[x][y][z].object = BIERRE;
                subgrids[x][y][z].texture = textures[BIERRE];
                printf("Le joueur a trouvé une bière et l'a mise dans la case %d\n", subgrids[x][y][z].id);
                break;
            case LOUPE:
                subgrids[x][y][z].object = LOUPE;
                subgrids[x][y][z].texture = textures[LOUPE];
                printf("Le joueur a trouvé une loupe et l'a mise dans la case %d\n", subgrids[x][y][z].id);
                break;
            case MENOTTES:
                subgrids[x][y][z].object = MENOTTES;
                subgrids[x][y][z].texture = textures[MENOTTES];
                printf("Le joueur a trouvé des menottes et les a mises dans la case %d\n", subgrids[x][y][z].id);
                break;
            default:
                printf("mauvais objet\n");
                break;
            }
        }
    }
}

// Initialize SDL and resources
bool initializeSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return false;
    }

    gWindow = SDL_CreateWindow("Main Menu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (gWindow == NULL)
    {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (gRenderer == NULL)
    {
        SDL_DestroyWindow(gWindow);
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    if (!loadMedia())
    {
        closeSDL();
        return false;
    }

    // Initialize button rectangles
    gLaunchButtonRect.x = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
    gLaunchButtonRect.y = WINDOW_HEIGHT / 2 - BUTTON_HEIGHT / 2;
    gLaunchButtonRect.w = BUTTON_WIDTH;
    gLaunchButtonRect.h = BUTTON_HEIGHT;

    gQuitButtonRect.x = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
    gQuitButtonRect.y = WINDOW_HEIGHT / 2 + BUTTON_HEIGHT / 2 + 20;
    gQuitButtonRect.w = BUTTON_WIDTH;
    gQuitButtonRect.h = BUTTON_HEIGHT;

    // Initialize title rectangle
    int titleWidth = 300;
    int titleHeight = 50;
    gTitleRect.x = (WINDOW_WIDTH - titleWidth) / 2;
    gTitleRect.y = (WINDOW_HEIGHT - titleHeight) / 4;
    gTitleRect.w = titleWidth;
    gTitleRect.h = titleHeight;

    return true;
}

// Load textures
bool loadMedia()
{
    gTitleTexture = loadTexture(gRenderer, "title.png");
    if (gTitleTexture == NULL)
    {
        fprintf(stderr, "Failed to load title texture!\n");
        return false;
    }

    gLaunchButtonTexture = loadTexture(gRenderer, "launch_button.png");
    if (gLaunchButtonTexture == NULL)
    {
        fprintf(stderr, "Failed to load launch button texture!\n");
        return false;
    }

    gQuitButtonTexture = loadTexture(gRenderer, "quit_button.png");
    if (gQuitButtonTexture == NULL)
    {
        fprintf(stderr, "Failed to load quit button texture!\n");
        return false;
    }

    return true;
}

// Close SDL and free resources
void closeSDL()
{
    SDL_DestroyTexture(gTitleTexture);
    SDL_DestroyTexture(gLaunchButtonTexture);
    SDL_DestroyTexture(gQuitButtonTexture);
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    SDL_Quit();
}

// Render title on the screen
void renderTitle()
{
    SDL_RenderCopy(gRenderer, gTitleTexture, NULL, &gTitleRect);
}

// Render buttons on the screen
void renderButtons()
{
    SDL_RenderCopy(gRenderer, gLaunchButtonTexture, NULL, &gLaunchButtonRect);
    SDL_RenderCopy(gRenderer, gQuitButtonTexture, NULL, &gQuitButtonRect);
}

// Render game content on the screen
bool renderGame()
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *imageTexture = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1)
    {
        printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("arial.ttf", 20);
    if (font == NULL)
    {
        printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    window = SDL_CreateWindow("Grille SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL)
    {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture *textures[4];
    textures[CIGARETTE] = loadTexture(renderer, "cigarette.png");
    textures[BIERRE] = loadTexture(renderer, "biere.png");
    textures[LOUPE] = loadTexture(renderer, "loupe.png");
    textures[MENOTTES] = loadTexture(renderer, "menottes.png");

    for (int i = 0; i < 4; ++i)
    {
        if (textures[i] == NULL)
        {
            for (int j = 0; j < 4; ++j)
            {
                if (textures[j] != NULL)
                {
                    SDL_DestroyTexture(textures[j]);
                }
            }
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
    }

    imageTexture = loadTexture(renderer, "pompe.png");
    if (imageTexture == NULL)
    {
        printf("Unable to create texture from image! SDL Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Rect imageRect;
    imageRect.x = CELL_WIDTH + CELL_WIDTH / 2 - CELL_WIDTH / 4;
    imageRect.y = CELL_HEIGHT / 2;
    imageRect.w = CELL_WIDTH / 2;
    imageRect.h = CELL_HEIGHT;

    GridCell grid[GRID_ROWS][GRID_COLS];
    int idCounter = 0;
    for (int i = 0; i < GRID_ROWS; ++i)
    {
        for (int j = 0; j < GRID_COLS; ++j)
        {
            grid[i][j].rect = (SDL_Rect){j * CELL_WIDTH, i * CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT};
            grid[i][j].clicked = false;
            grid[i][j].id = idCounter++;
            grid[i][j].texture = NULL;
        }
    }

    GridCell subgrids[4][SUBGRID_ROWS][SUBGRID_COLS];
    SDL_Rect subgrid_positions[4] = {
        {0, 0, CELL_WIDTH, CELL_HEIGHT},
        {CELL_WIDTH * 2, 0, CELL_WIDTH, CELL_HEIGHT},
        {0, CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT},
        {CELL_WIDTH * 2, CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT}};

    for (int g = 0; g < 4; ++g)
    {
        for (int i = 0; i < SUBGRID_ROWS; ++i)
        {
            for (int j = 0; j < SUBGRID_COLS; ++j)
            {
                subgrids[g][i][j].rect = (SDL_Rect){
                    subgrid_positions[g].x + SUBGRID_MARGIN + j * (SUBGRID_CELL_WIDTH + SUBGRID_MARGIN),
                    subgrid_positions[g].y + SUBGRID_MARGIN + i * (SUBGRID_CELL_HEIGHT + SUBGRID_MARGIN),
                    SUBGRID_CELL_WIDTH,
                    SUBGRID_CELL_HEIGHT};
                subgrids[g][i][j].clicked = false;
                subgrids[g][i][j].id = idCounter++;
                subgrids[g][i][j].texture = NULL;
            }
        }
    }

    GridCell extraCells[2] = {
        {{SCREEN_WIDTH - 100, 0, 100, SCREEN_HEIGHT / 2}, false, idCounter++, Null, NULL},
        {{SCREEN_WIDTH - 100, SCREEN_HEIGHT / 2, 100, SCREEN_HEIGHT / 2}, false, idCounter++, Null, NULL}};

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // Initialiser les variables
    int balles[8];
    int rouges = 0, noirs = 0, nombreDeBalles = 0;
    SDL_Event e;
    bool quitGame = false;
    int vieJoueur, vieOrdi;
    bool joueurDoitRejouer = false;
    bool ordinateurDoitRejouer = false;
    bool joueurTurn = true;
    int x, y;
    int manche = 1;

    drawGrid(renderer, grid, subgrids, extraCells, font, rouges, noirs, nombreDeBalles, manche, vieJoueur, vieOrdi);

    SDL_RenderPresent(renderer);

    srand(time(NULL));

    bool quit = false;

    while (!quitGame && manche <= 3)
    {
        printf("while quit/manche\n");
        vieJoueur = 3 + rand() % 4; // Vie du joueur entre 3 et 6
        vieOrdi = vieJoueur;
        printf("Manche %d: Vie Joueur = %d, Vie Ordi = %d\n", manche, vieJoueur, vieOrdi);

        genererBalles(balles, &rouges, &noirs, &nombreDeBalles);
        giveObject(subgrids, textures);

        while (!quitGame && (vieJoueur > 0 && vieOrdi > 0))
        {
            while (SDL_PollEvent(&e) != 0)
            {
                if (e.type == SDL_QUIT)
                {
                    quitGame = true;
                }
                else if (e.type == SDL_MOUSEBUTTONDOWN)
                {
                    SDL_GetMouseState(&x, &y);
                    int idCase = handleMouseClick(x, y, grid, subgrids, extraCells, renderer);
                    printf("ID de la case cliquée: %d\n", idCase);
                    if (joueurTurn)
                    {
                        if (idCase == 1 || idCase == 4)
                        {
                            joueurDoitRejouer = joueurTour(idCase, &vieJoueur, &vieOrdi, &rouges, &noirs, &nombreDeBalles, balles);
                            if (!joueurDoitRejouer)
                            {
                                joueurTurn = false;
                            }
                        }
                        else if (idCase >= 14 && idCase <= 21)
                        {
                            printf("Object");
                            int x, y, z;
                            // si la case qui a été cliqué est une case avec un objet alors on enleve l'objet de la case et on utilise l'objet
                            if (idCase >= 14 && idCase <= 17)
                            {
                                x = 2;
                                if (idCase == 14 || idCase == 15)
                                {
                                    y = 0;
                                }
                                else if (idCase == 16 || idCase == 17)
                                {
                                    y = 1;
                                }
                                if (idCase == 14 || idCase == 16)
                                {
                                    z = 0;
                                }
                                else if (idCase == 15 || idCase == 17)
                                {
                                    z = 1;
                                }
                                printf("x = %d, y = %d, z = %d\n", x, y, z);
                            }
                            else if (idCase >= 18 && idCase <= 21)
                            {
                                x = 3;
                                if (idCase == 18 || idCase == 19)
                                {
                                    y = 0;
                                }
                                else if (idCase == 20 || idCase == 21)
                                {
                                    y = 1;
                                }
                                if (idCase == 18 || idCase == 20)
                                {
                                    z = 0;
                                }
                                else if (idCase == 19 || idCase == 21)
                                {
                                    z = 1;
                                }
                                printf("x = %d, y = %d, z = %d\n", x, y, z);
                            }
                            if (subgrids[x][y][z].object == CIGARETTE)
                            {
                                printf("le joueur a utilisé une cigarette et a gagné une vie\n");
                                vieJoueur++;
                            }
                            else if (subgrids[x][y][z].object == BIERRE)
                            {
                                printf("le joueur a utilisé une bière et passe donc a la balle suivante\n");
                                if (balles[0] == ROUGE)
                                {
                                    rouges--;
                                }
                                else
                                {
                                    noirs--;
                                }
                                for (int i = 0; i < nombreDeBalles; i++)
                                {
                                    balles[i] = balles[i + 1];
                                }
                                balles[nombreDeBalles] = -1;
                                nombreDeBalles--;
                            }
                            else if (subgrids[x][y][z].object == LOUPE)
                            {
                                printf("le joueur a utilisé une loupe et a vu la couleur de la prochaine balle\n");
                                printf("La prochaine balle est %s\n", balles[0] == ROUGE ? "Rouge" : "Noir");
                            }
                            else if (subgrids[x][y][z].object == MENOTTES)
                            {
                                printf("le joueur a utilisé des menottes et a bloqué l'ordinateur pour un tour\n");
                                joueurTurn = true;
                            }
                            subgrids[x][y][z].object = Null;
                            subgrids[x][y][z].texture = NULL;
                        }
                    }
                }
            }

            if (nombreDeBalles == 0)
            {
                genererBalles(balles, &rouges, &noirs, &nombreDeBalles);
                giveObject(subgrids, textures);
            }

            // Logique pour l'ordinateur
            if (!joueurTurn)
            {
                ordinateurDoitRejouer = ordinateurTour(&vieJoueur, &vieOrdi, &rouges, &noirs, &nombreDeBalles, balles);
                joueurTurn = true;
            }

            if (nombreDeBalles == 0)
            {
                genererBalles(balles, &rouges, &noirs, &nombreDeBalles);
                giveObject(subgrids, textures);
            }

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderClear(renderer);
            drawGrid(renderer, grid, subgrids, extraCells, font, rouges, noirs, nombreDeBalles, manche, vieJoueur, vieOrdi);
            SDL_RenderCopy(renderer, imageTexture, NULL, &imageRect);
            SDL_RenderPresent(renderer);
        }

        if (vieJoueur <= 0)
        {
            printf("Vous avez perdu face au Dealer %d.\n", manche);
            quitGame = true;
        }
        else
        {
            manche++;
        }
    }

    if (manche > 3 && vieJoueur > 0)
    {
        printf("Vous avez gagné les 3 manches !\n");
    }

    // Libération des ressources
    SDL_DestroyTexture(imageTexture);
    for (int i = 0; i < 4; ++i)
    {
        if (textures[i] != NULL)
        {
            SDL_DestroyTexture(textures[i]);
        }
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();

    return quitGame;
}

void renderMenu()
{
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
    SDL_RenderClear(gRenderer);
    renderTitle();
    renderButtons();
    SDL_RenderPresent(gRenderer);
}

int main(int argc, char *argv[])
{
    if (!initializeSDL())
    {
        return 1;
    }

    SDL_Event e;
    int currentState = STATE_MENU;

    while (currentState != STATE_QUIT)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                currentState = STATE_QUIT;
            }
        }

        switch (currentState)
        {
        case STATE_MENU:
            renderMenu();
            while (SDL_PollEvent(&e))
            {
                if (e.type == SDL_QUIT)
                {
                    currentState = STATE_QUIT;
                }
                else if (e.type == SDL_MOUSEBUTTONDOWN)
                {
                    int mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);

                    if (mouseX >= gLaunchButtonRect.x && mouseX <= (gLaunchButtonRect.x + gLaunchButtonRect.w) &&
                        mouseY >= gLaunchButtonRect.y && mouseY <= (gLaunchButtonRect.y + gLaunchButtonRect.h))
                    {
                        currentState = STATE_GAME;
                    }

                    if (mouseX >= gQuitButtonRect.x && mouseX <= (gQuitButtonRect.x + gQuitButtonRect.w) &&
                        mouseY >= gQuitButtonRect.y && mouseY <= (gQuitButtonRect.y + gQuitButtonRect.h))
                    {
                        currentState = STATE_QUIT;
                    }
                }
            }
            break;

        case STATE_GAME:
            if (renderGame())
            {
                currentState = STATE_QUIT;
            }
            else
            {
                currentState = STATE_MENU;
            }
            break;

        case STATE_QUIT:
            break;
        }
    }

    closeSDL();
    return 0;
}
