#include <SDL3/SDL.h>
#include <SDL3/SDL_ttf.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define CELL_SIZE 40
#define MARGIN 20
#define FONT_SIZE 24

typedef struct {
    int width;
    int height;
    int mines;
    int flags;
    int revealed;
    bool game_over;
    bool victory;
    bool first_click;
    time_t start_time;
} GameState;

typedef struct {
    bool is_mine;
    bool is_revealed;
    bool is_flagged;
    int neighbor_mines;
} Cell;

// Difficultés prédéfinies
const GameState difficulties[] = {
    {8, 8, 10},   // Débutant
    {12, 12, 20},  // Intermédiaire
    {16, 16, 40}   // Expert
};

// Variables globales
SDL_Window* window;
SDL_Renderer* renderer;
TTF_Font* font;
Cell** grid;
GameState state;

void init_game(int difficulty) {
    state = difficulties[difficulty];
    state.flags = 0;
    state.revealed = 0;
    state.game_over = false;
    state.victory = false;
    state.first_click = true;

    // Redimensionner la fenêtre
    SDL_SetWindowSize(window, 
        state.width * CELL_SIZE + 2*MARGIN, 
        state.height * CELL_SIZE + 2*MARGIN + FONT_SIZE*2);

    // Allouer la grille
    grid = realloc(grid, state.height * sizeof(Cell*));
    for(int y = 0; y < state.height; y++) {
        grid[y] = realloc(grid[y], state.width * sizeof(Cell));
        for(int x = 0; x < state.width; x++) {
            grid[y][x] = (Cell){false, false, false, 0};
        }
    }
}

void place_mines(int safe_x, int safe_y) {
    srand(time(NULL));
    int mines = 0;
    
    while(mines < state.mines) {
        int x = rand() % state.width;
        int y = rand() % state.height;
        
        if((abs(x - safe_x) > 1 || abs(y - safe_y) > 1) {
            if(!grid[y][x].is_mine) {
                grid[y][x].is_mine = true;
                mines++;
            }
        }
    }
}

void calculate_numbers() {
    for(int y = 0; y < state.height; y++) {
        for(int x = 0; x < state.width; x++) {
            if(!grid[y][x].is_mine) {
                int count = 0;
                for(int dy = -1; dy <= 1; dy++) {
                    for(int dx = -1; dx <= 1; dx++) {
                        int nx = x + dx;
                        int ny = y + dy;
                        if(nx >= 0 && nx < state.width && ny >= 0 && ny < state.height) {
                            if(grid[ny][nx].is_mine) count++;
                        }
                    }
                }
                grid[y][x].neighbor_mines = count;
            }
        }
    }
}

void reveal(int x, int y) {
    if(x < 0 || x >= state.width || y < 0 || y >= state.height) return;
    if(grid[y][x].is_revealed || grid[y][x].is_flagged) return;
    
    grid[y][x].is_revealed = true;
    state.revealed++;
    
    if(grid[y][x].neighbor_mines == 0) {
        for(int dy = -1; dy <= 1; dy++) {
            for(int dx = -1; dx <= 1; dx++) {
                reveal(x + dx, y + dy);
            }
        }
    }
}

void render_text(const char* text, int x, int y, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderTexture(renderer, texture, NULL, &rect);
    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);
}

void render_cell(int x, int y) {
    SDL_FRect rect = {
        MARGIN + x * CELL_SIZE,
        MARGIN + FONT_SIZE*2 + y * CELL_SIZE,
        CELL_SIZE - 1,
        CELL_SIZE - 1
    };

    // Couleurs
    if(grid[y][x].is_revealed) {
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        if(grid[y][x].is_mine && state.game_over) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        }
    } else {
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    }
    
    SDL_RenderRect(renderer, &rect);
    
    if(grid[y][x].is_flagged) {
        render_text("🚩", rect.x + 5, rect.y + 5, (SDL_Color){255, 0, 0, 255});
    }
    else if(grid[y][x].is_revealed && grid[y][x].neighbor_mines > 0) {
        char num[2] = {grid[y][x].neighbor_mines + '0', '\0'};
        SDL_Color colors[] = {{0,0,255}, {0,128,0}, {255,0,0}, {0,0,128}, {128,0,0}, {0,128,128}, {0,0,0}, {128,128,128}};
        render_text(num, rect.x + 15, rect.y + 10, colors[grid[y][x].neighbor_mines - 1]);
    }
}

void render() {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // Infos
    char info[50];
    SDL_Color black = {0, 0, 0, 255};
    sprintf(info, "Mines: %d/%d  Temps: %ld", state.flags, state.mines, time(NULL) - state.start_time);
    render_text(info, MARGIN, MARGIN, black);

    // Grille
    for(int y = 0; y < state.height; y++) {
        for(int x = 0; x < state.width; x++) {
            render_cell(x, y);
        }
    }

    // Message de fin
    if(state.game_over) {
        const char* message = state.victory ? "Victoire !" : "Game Over !";
        render_text(message, MARGIN, MARGIN + FONT_SIZE, (SDL_Color){255, 0, 0, 255});
    }

    SDL_RenderPresent(renderer);
}

void handle_click(int x, int y, bool right_click) {
    int grid_x = (x - MARGIN) / CELL_SIZE;
    int grid_y = (y - MARGIN - FONT_SIZE*2) / CELL_SIZE;
    
    if(grid_x < 0 || grid_x >= state.width || grid_y < 0 || grid_y >= state.height) return;

    if(state.first_click) {
        place_mines(grid_x, grid_y);
        calculate_numbers();
        state.start_time = time(NULL);
        state.first_click = false;
    }

    if(right_click) {
        if(!grid[grid_y][grid_x].is_revealed) {
            grid[grid_y][grid_x].is_flagged = !grid[grid_y][grid_x].is_flagged;
            state.flags += grid[grid_y][grid_x].is_flagged ? 1 : -1;
        }
    } else {
        if(grid[grid_y][grid_x].is_mine) {
            state.game_over = true;
            state.victory = false;
        } else {
            reveal(grid_x, grid_y);
            if(state.revealed == state.width * state.height - state.mines) {
                state.game_over = true;
                state.victory = true;
            }
        }
    }
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    window = SDL_CreateWindow("Démineur SDL3", 640, 480, 0);
    renderer = SDL_CreateRenderer(window, NULL, SDL_RENDERER_ACCELERATED);
    font = TTF_OpenFont("arial.ttf", FONT_SIZE);

    init_game(0); // Difficulté par défaut

    SDL_Event event;
    bool running = true;
    
    while(running) {
        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if(!state.game_over) {
                        int x, y;
                        SDL_GetMouseState(&x, &y);
                        handle_click(x, y, event.button.button == SDL_BUTTON_RIGHT);
                    }
                    break;
                
                case SDL_EVENT_KEY_DOWN:
                    if(state.game_over) {
                        int key = event.key.keysym.sym;
                        if(key >= SDLK_1 && key <= SDLK_3) {
                            init_game(key - SDLK_1);
                        }
                    }
                    break;
            }
        }
        render();
        SDL_Delay(16);
    }

    for(int y = 0; y < state.height; y++) free(grid[y]);
    free(grid);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
