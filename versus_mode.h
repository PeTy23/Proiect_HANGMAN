#ifndef __VERSUS_MODE__
#define __VERSUS_MODE__

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "normal_mode.h" // For HangmanGame struct and related defines
#include "interface.h"   // For Game struct

#define INITIAL_VERSUS_MODE_TIME_SECONDS 30
#define TIME_BONUS_GUESS_SECONDS 15

// Enum to define which player's turn it is
typedef enum {
    PLAYER_1,
    PLAYER_2
} CurrentPlayer;

// Struct to hold data for the versus mode
typedef struct VersusHangman {
    HangmanGame player1;
    HangmanGame player2;
    CurrentPlayer current_turn;
    int common_word_length; // To ensure both players get words of the same length
    long round_over_display_time; // To control how long game over/win messages are shown
    bool overall_game_over_by_time;
} VersusHangman;

// Declare external functions used from normal_mode.c and interface.c
extern bool normal_mode_load_words_from_file(HangmanGame* hangman, GameLanguage lang);
extern char* normal_mode_get_random_word_of_length(HangmanGame* hangman, int length);
extern void render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Color color, int x, int y);
//extern void render_hangman_image(SDL_Renderer* renderer, int wrong_guesses, int x_offset, int y_offset);

// Function declarations for Versus Mode
void versus_mode_init(Game* game);
void versus_mode_cleanup(Game* game);
void versus_mode_reset(Game* game, bool full_game_reset);
void versus_mode_handle_event(Game* game, SDL_Event* event);
void versus_mode_render(Game* game);

// Helper functions for Versus Mode (internal to versus_mode.c)
void versus_mode_process_key(Game* game, char key);
void versus_mode_update_displayed_word(HangmanGame* hangman);

#endif // __VERSUS_MODE__