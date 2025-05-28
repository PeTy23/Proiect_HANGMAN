#ifndef __INTERFACE__
#define __INTERFACE__

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

#define WIDTH 1000
#define HEIGHT 800
#define FONT_SIZE 40 
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef enum {
    LANG_ENGLISH,
    LANG_ROMANIAN,
    LANG_COUNT 
} GameLanguage;

typedef enum {
    MAIN_MENU,
    NORMAL_MODE,
    HARD_MODE, 
    VERSUS_MODE,
} GameState;

typedef struct Button {
    SDL_Rect rect; 
    SDL_Texture* texture; 
    char text[50]; 
    bool is_hovered;
} Button; 

typedef enum {
    BUTTON_NORMAL_MODE,
    BUTTON_HARD_MODE,
    BUTTON_VERSUS_MODE,
    BUTTON_LANGUAGE,
    BUTTON_COUNT
} ButtonType;

typedef struct HangmanGame HangmanGame; 
typedef struct VersusHangman VersusHangman;

typedef struct Game {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* background;
    TTF_Font* text_font;
    SDL_Color text_color;
    GameState current_state;
    Button buttons[BUTTON_COUNT];
    HangmanGame* hangman; 
    VersusHangman* versus_data;
    char temp_message[256];

    GameLanguage current_language; 
    SDL_Texture* flag_en_texture;  
    SDL_Texture* flag_ro_texture;  
    SDL_Rect flag_rect;            
} Game;

bool initialize_game(Game* game);
void cleanup_game(Game* game);
bool load_media(Game* game);
void handle_events(Game* game);
void render_main_menu(Game* game);
//void render_mode_under_construction(Game* game); 
void render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Color color, int x, int y);
void render_hangman_image(SDL_Renderer* renderer, int wrong_guesses, int x_offset, int y_offset, bool mirrored);
void render_keyboard(Game* game); 

void versus_mode_init(Game* game);
void versus_mode_cleanup(Game* game);
void versus_mode_reset(Game* game, bool full_game_reset);
void versus_mode_handle_event(Game* game, SDL_Event* event);
void versus_mode_render(Game* game);

void versus_mode_process_key(Game* game, char key);
void versus_mode_update_displayed_word(HangmanGame* hangman);

#endif // __INTERFACE__