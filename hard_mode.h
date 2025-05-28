#ifndef __HARD_MODE__
#define __HARD_MODE__

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "interface.h"   // For Game struct, WIDTH, HEIGHT, and generic rendering function declarations
#include "normal_mode.h" // For HangmanGame struct and defines like MAX_WORD_LENGTH, ALPHABET_SIZE, etc.

// Function declarations for Hard Mode
void hard_mode_init(Game* game);
void hard_mode_cleanup(Game* game);
void hard_mode_reset(Game* game);
void hard_mode_handle_event(Game* game, SDL_Event* event);
void hard_mode_render(Game* game);

// Helper functions specific to Hard Mode logic
char* hard_mode_get_random_word_by_length(HangmanGame* hangman, int length);
void hard_mode_process_key(Game* game, char key);
void hard_mode_update_displayed_word(Game* game);

#endif // __HARD_MODE__
