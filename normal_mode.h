#ifndef __NORMAL_MODE__
#define __NORMAL_MODE__

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include "interface.h"

#define MAX_WORD_LENGTH 30
#define MAX_WRONG_GUESSES 6
#define ALPHABET_SIZE 26
#define KEYBOARD_ROWS 2
#define KEYBOARD_COLS 13
#define KEY_WIDTH 40
#define KEY_HEIGHT 40
#define KEY_SPACING 10
#define KEYBOARD_START_X 180
#define KEYBOARD_START_Y 650

#define INITIAL_HARD_MODE_TIME_SECONDS 40 
#define TIME_BONUS_WIN_SECONDS 20       
#define INITIAL_WORD_LENGTH 3            
#define MAX_GAME_WORD_LENGTH 10
#define WRONG_GUESS_BONUS_WIN 2   
#define WORDS_TO_WIN_VERSUS_MODE 7 
#define GUESS_BONUS_WORD_GUESSED 4   
#define GUESS_BONUS_ROUND_LOST 2

typedef struct Game Game;

typedef struct HangmanGame {
    char word[MAX_WORD_LENGTH + 1];
    char displayed_word[MAX_WORD_LENGTH * 2 + 1]; //pt litera si space
    bool guessed_letters[ALPHABET_SIZE];
    int wrong_guesses;
    bool game_over;
    bool win;
    SDL_Texture* letter_textures[ALPHABET_SIZE];
    SDL_Rect letter_rects[ALPHABET_SIZE];
    char** word_list_dynamic;
    int word_count_dynamic;
    long start_time_ms;          
    long time_left_ms;          
    long current_round_time_limit_ms; 
    int current_word_length;
    bool win_previous_round; 
    long round_won_display_time;
    int words_guessed_count; 
}HangmanGame;

void normal_mode_init(Game* game);
void normal_mode_cleanup(Game* game);
void normal_mode_reset(Game* game);
void normal_mode_handle_event(Game* game, SDL_Event* event);
void normal_mode_render(Game* game);
char* normal_mode_get_random_word();
void normal_mode_process_key(Game* game, char key);
void normal_mode_update_displayed_word(Game* game);
//void render_hangman_figure(Game* game);
//void render_keyboard(Game* game);
void render_game_over_message(Game* game);
bool normal_mode_load_words_from_file(HangmanGame* hangman, GameLanguage lang);


#endif 