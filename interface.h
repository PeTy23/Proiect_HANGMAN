#ifndef __GAME__
#define __GAME__

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

typedef enum {
    MAIN_MENU,
    NORMAL_MODE,
    HARD_MODE,
    VERSUS_MODE,
    LANGUAGE
} GameState; //fiecare state al jocului

typedef struct Button {
    SDL_Rect rect; //SDL_RECT contine x,y(coordonatele din stanga) si w,h(width,height)
    SDL_Texture* texture;
    char text[50];
    bool is_hovered;
} Button; //fiecare element al butonului

typedef enum {
    BUTTON_NORMAL_MODE,
    BUTTON_HARD_MODE,
    BUTTON_VERSUS_MODE,
    BUTTON_LANGUAGE,
    BUTTON_COUNT 
} ButtonType; 

typedef struct Game {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* background;
    TTF_Font* text_font;
    SDL_Color text_color;
    GameState current_state;
    Button buttons[BUTTON_COUNT];
    SDL_Texture* hangman_texture;
} Game;

bool initialize_game(Game* game);
void cleanup_game(Game* game);
void handle_events(Game* game);
bool load_media(Game* game);
void render_main_menu(Game* game);
void render_normal_mode(Game* game);
Button create_button(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y);
void destroy_button(Button* button);
void render_button(SDL_Renderer* renderer, Button* button);

#endif