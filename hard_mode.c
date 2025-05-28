// hard_mode.c - Implementation of the hard mode hangman game

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h> // For M_PI if needed for drawing
#include <errno.h> // For strerror
#include "hard_mode.h" // Include its own header first
#include "interface.h" // For Game struct and rendering helpers (WIDTH, HEIGHT, render_text, render_hangman_image, FONT_SIZE)
#include "normal_mode.h" // For HangmanGame struct and defines like MAX_WORD_LENGTH etc.

// Define M_PI explicitly if it's not defined by <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Define the filename for your word list (reusing the one from normal_mode)
#define WORDLIST_FILENAME "words.txt"

// Define the duration for which "WORD GUESSED!" message is displayed (in milliseconds)
#define ROUND_WIN_DISPLAY_DURATION 1500 // 1.5 seconds

// --- Helper Functions ---

// Function to load words from a file (reusing normal_mode's function)
// This function is already present in normal_mode.c, so we just declare it here
// and use it. No need to redefine it.
extern bool normal_mode_load_words_from_file(HangmanGame* hangman, GameLanguage lang);

// Explicit declaration for render_keyboard to resolve potential implicit declaration warnings
//extern void render_keyboard(Game* game);


// Get a random word from the dynamically loaded word list based on desired length
// This will be crucial for the progressive word length feature
char* hard_mode_get_random_word_by_length(HangmanGame* hangman, int length) {
    fprintf(stderr, "DEBUG: hard_mode_get_random_word_by_length called for length %d.\n", length);
    if (hangman == NULL || hangman->word_count_dynamic == 0 || hangman->word_list_dynamic == NULL) {
        fprintf(stderr, "ERROR: hard_mode_get_random_word_by_length: Word list not loaded or empty (hangman is NULL or data missing).\n");
        return "ERROR"; // Return a default or error word
    }

    // VLA - Variable Length Array. Be cautious with very large word lists.
    // For extreme cases, dynamic allocation (malloc) would be safer.
    char* suitable_words[hangman->word_count_dynamic];
    int suitable_count = 0;
    for (int i = 0; i < hangman->word_count_dynamic; i++) {
        if (strlen(hangman->word_list_dynamic[i]) == length) {
            suitable_words[suitable_count++] = hangman->word_list_dynamic[i];
        }
    }

    if (suitable_count == 0) {
        fprintf(stderr, "WARNING: hard_mode_get_random_word_by_length: No words found of length %d in word list. Falling back to any word.\n", length);
        if (hangman->word_count_dynamic > 0) {
            return hangman->word_list_dynamic[rand() % hangman->word_count_dynamic];
        }
        fprintf(stderr, "ERROR: hard_mode_get_random_word_by_length: No words available at all in word list.\n");
        return "DEFAULT"; // Ultimate fallback if word list is empty
    }

    int idx = rand() % suitable_count;
    fprintf(stderr, "DEBUG: hard_mode_get_random_word_by_length: Selected word of length %d: %s\n", length, suitable_words[idx]);
    return suitable_words[idx];
}


// Process keyboard input for hard mode
void hard_mode_process_key(Game* game, char key) {
    fprintf(stderr, "DEBUG: hard_mode_process_key called with key '%c'.\n", key);
    if (game == NULL || game->hangman == NULL) {
        fprintf(stderr, "ERROR: hard_mode_process_key: game or game->hangman is NULL. Cannot process key.\n");
        return;
    }

    // If the game is definitively over (lost or overall won),
    // a key press should trigger a full reset.
    // If it's a round win (game->hangman->win is true, but game->hangman->game_over is false),
    // we don't process key presses, but let the render loop handle the transition.
    if (game->hangman->game_over || game->hangman->win) { // If game is over OR a round is won (waiting for display)
        if (game->hangman->game_over) { // Only reset if truly game over (lost or overall won)
            fprintf(stderr, "DEBUG: hard_mode_process_key: Game is over, resetting.\n");
            hard_mode_reset(game);
        }
        return; // Ignore key presses during round win display or definitive game over
    }
    
    if (key >= 'a' && key <= 'z') {
        key = key - 32; // Convert to uppercase
    }

    if (key >= 'A' && key <= 'Z') {
        int idx = key - 'A';
        
        if (game->hangman->guessed_letters[idx]) {
            fprintf(stderr, "DEBUG: hard_mode_process_key: Letter '%c' already guessed.\n", key);
            return;
        }
        
        game->hangman->guessed_letters[idx] = true;
        
        bool found = false;
        for (size_t i = 0; i < strlen(game->hangman->word); i++) {
            if (game->hangman->word[i] == key) {
                found = true;
                break; // Found the letter, no need to check further
            }
        }
        
        if (!found) {
            game->hangman->wrong_guesses++;
            fprintf(stderr, "DEBUG: hard_mode_process_key: Incorrect guess '%c'. Wrong guesses: %d\n", key, game->hangman->wrong_guesses);
        } else {
            fprintf(stderr, "DEBUG: hard_mode_process_key: Correct guess '%c'.\n", key);
        }
        
        hard_mode_update_displayed_word(game);
    } else {
        fprintf(stderr, "DEBUG: hard_mode_process_key: Non-alphabetical key '%c' ignored.\n", key);
    }
}

// Update the displayed word with correct guesses and underscores
// This function also checks for win/loss conditions and applies bonuses
void hard_mode_update_displayed_word(Game* game) {
    fprintf(stderr, "DEBUG: hard_mode_update_displayed_word called.\n");
    if (game == NULL || game->hangman == NULL) {
        fprintf(stderr, "ERROR: hard_mode_update_displayed_word: game or game->hangman is NULL. Cannot update displayed word.\n");
        return;
    }
    size_t word_len = strlen(game->hangman->word);
    char* display = game->hangman->displayed_word;
    
    display[0] = '\0'; // Clear the displayed word buffer
    
    for (size_t i = 0; i < word_len; i++) {
        char c = game->hangman->word[i];
        int idx = c - 'A';
        
        if (game->hangman->guessed_letters[idx]) {
            char temp[4] = {0}; // Buffer for character and space, plus null terminator
            sprintf(temp, "%c ", c);
            strcat(display, temp);
        } else {
            strcat(display, "_ ");
        }
    }
    fprintf(stderr, "DEBUG: Displayed word updated to: %s\n", display);
    
    // Check if all letters have been guessed (win condition for the current word/round)
    bool all_guessed = true;
    for (size_t i = 0; i < word_len; i++) {
        char c = game->hangman->word[i];
        int idx = c - 'A';
        if (!game->hangman->guessed_letters[idx]) {
            all_guessed = false;
            break;
        }
    }
    
    // Update game state based on progress
    if (all_guessed) {
        fprintf(stderr, "DEBUG: hard_mode_update_displayed_word: Word guessed correctly! Setting win flag.\n");
        game->hangman->win = true; // Set win flag for this round
        // Do NOT set game_over = true here. This allows the render loop to handle the delay.
        game->hangman->win_previous_round = true; // Mark previous round as a win

        // Apply time bonus: add 1 minute to the current round's total time limit
        game->hangman->current_round_time_limit_ms += (TIME_BONUS_WIN_SECONDS * 1000);
        fprintf(stderr, "DEBUG: Time bonus applied. New time limit: %ld ms.\n", game->hangman->current_round_time_limit_ms);
        
        // Apply guess bonus: add 2 guesses (subtract from wrong_guesses)
        game->hangman->wrong_guesses -= WRONG_GUESS_BONUS_WIN;
        if (game->hangman->wrong_guesses < 0) {
            game->hangman->wrong_guesses = 0;
        }
        fprintf(stderr, "DEBUG: Guess bonus applied. New wrong guesses: %d.\n", game->hangman->wrong_guesses);

        // Record the time when the round was won, for the display delay
        game->hangman->round_won_display_time = SDL_GetTicks();
        fprintf(stderr, "DEBUG: Round won display time set to %ld.\n", game->hangman->round_won_display_time);
        
    } else if (game->hangman->wrong_guesses >= MAX_WRONG_GUESSES) {
        fprintf(stderr, "DEBUG: hard_mode_update_displayed_word: Max wrong guesses reached. Game Over.\n");
        game->hangman->game_over = true; // This is a definitive game over
        game->hangman->win = false;
        game->hangman->win_previous_round = false;
    }
}


// Reset the game for a new round in Hard Mode
void hard_mode_reset(Game* game) {
    fprintf(stderr, "DEBUG: hard_mode_reset called.\n");
    if (game == NULL || game->hangman == NULL) {
        fprintf(stderr, "ERROR: hard_mode_reset: game or game->hangman is NULL. Cannot reset.\n");
        return;
    }
    bool overall_game_won_this_reset = false; // Flag to track if the player achieved the ultimate win in THIS reset call

    // Determine the next word length
    if (game->hangman->win_previous_round) {
        if (game->hangman->current_word_length < MAX_GAME_WORD_LENGTH) {
            game->hangman->current_word_length++;
            fprintf(stderr, "DEBUG: hard_mode_reset: Previous round was a win, incrementing word length to %d.\n", game->hangman->current_word_length);
        } else {
            // Player has guessed the final MAX_GAME_WORD_LENGTH-letter word (overall game win)
            overall_game_won_this_reset = true; // Set overall win flag
            game->hangman->current_word_length = INITIAL_WORD_LENGTH; // Reset for a new playthrough if they click again
            fprintf(stderr, "DEBUG: hard_mode_reset: Max word length reached (overall win). Resetting length to %d.\n", game->hangman->current_word_length);
        }
    } else {
        // Lost previous round or first game, reset to initial length
        game->hangman->current_word_length = INITIAL_WORD_LENGTH;
        fprintf(stderr, "DEBUG: hard_mode_reset: Previous round was a loss or first game. Resetting length to %d.\n", game->hangman->current_word_length);
    }

    // If the overall game was won, set game_over and win flags and stop here.
    // The render function will then display the "CONGRATULATIONS" message.
    if (overall_game_won_this_reset) {
        game->hangman->game_over = true; // This is the definitive game over for overall win
        game->hangman->win = true;       // True for overall game win
        game->hangman->win_previous_round = false; // Reset for next potential game
        fprintf(stderr, "DEBUG: hard_mode_reset: Overall game won! Setting game_over and win flags.\n");
        return; // Exit reset function, no new word loaded, wait for user click
    }

    const char* chosen_word = hard_mode_get_random_word_by_length(game->hangman, game->hangman->current_word_length);
    if (strcmp(chosen_word, "ERROR") == 0 || strcmp(chosen_word, "DEFAULT") == 0) {
        fprintf(stderr, "ERROR: hard_mode_reset: Failed to get a suitable word. Cannot reset game.\n");
        game->hangman->game_over = true; // Mark as game over due to error
        game->hangman->win = false;
        return;
    }
    strncpy(game->hangman->word, chosen_word, MAX_WORD_LENGTH);
    game->hangman->word[MAX_WORD_LENGTH] = '\0';
    fprintf(stderr, "DEBUG: hard_mode_reset: New word chosen: %s\n", game->hangman->word);
    
    memset(game->hangman->guessed_letters, 0, sizeof(game->hangman->guessed_letters));
    game->hangman->wrong_guesses = 0;
    game->hangman->game_over = false; // Ensure not game over for a new round
    game->hangman->win = false;       // Reset win status for new round (for this new round)
    game->hangman->round_won_display_time = 0; // Reset display timer for new round

    game->hangman->start_time_ms = SDL_GetTicks();
    // If it was a win, current_round_time_limit_ms already has the bonus added from hard_mode_update_displayed_word.
    // If it was a loss, or a fresh start (overall win), reset to initial time.
    if (!game->hangman->win_previous_round || game->hangman->current_round_time_limit_ms == 0) {
        game->hangman->current_round_time_limit_ms = INITIAL_HARD_MODE_TIME_SECONDS * 1000;
        fprintf(stderr, "DEBUG: hard_mode_reset: Resetting time limit to initial: %ld ms.\n", game->hangman->current_round_time_limit_ms);
    } else {
        fprintf(stderr, "DEBUG: hard_mode_reset: Carrying over time limit: %ld ms.\n", game->hangman->current_round_time_limit_ms);
    }
    game->hangman->time_left_ms = game->hangman->current_round_time_limit_ms;

    game->hangman->win_previous_round = false; // Reset for the next round's check

    hard_mode_update_displayed_word(game); // This will set up the underscores for the new word
    fprintf(stderr, "DEBUG: hard_mode_reset completed. New word length: %d, Word: %s\n", game->hangman->current_word_length, game->hangman->word);
}


// Initialize the hard mode game
void hard_mode_init(Game* game) {
    fprintf(stderr, "DEBUG: hard_mode_init called.\n");
    if (game == NULL) {
        fprintf(stderr, "ERROR: hard_mode_init: Game pointer is NULL. Aborting initialization.\n");
        return;
    }
    if (game->hangman != NULL) {
        fprintf(stderr, "DEBUG: hard_mode_init: Cleaning up existing hangman data.\n");
        hard_mode_cleanup(game);
    }

    game->hangman = malloc(sizeof(HangmanGame));
    if (!game->hangman) {
        fprintf(stderr, "ERROR: hard_mode_init: Failed to allocate memory for Hangman game: %s\n", strerror(errno));
        game->hangman = NULL;
        return;
    }
    fprintf(stderr, "DEBUG: hard_mode_init: HangmanGame struct allocated at %p.\n", (void*)game->hangman);
    
    game->hangman->word_list_dynamic = NULL;
    game->hangman->word_count_dynamic = 0;

    srand(time(NULL));
    
    if (!normal_mode_load_words_from_file(game->hangman, game->current_language)) {
        fprintf(stderr, "ERROR: hard_mode_init: Failed to load words from file");
        // normal_mode_load_words_from_file already frees game->hangman and sets to NULL on failure.
        return;
    }
    fprintf(stderr, "DEBUG: hard_mode_init: Words loaded successfully.\n");

    game->hangman->current_word_length = INITIAL_WORD_LENGTH;
    game->hangman->current_round_time_limit_ms = INITIAL_HARD_MODE_TIME_SECONDS * 1000;
    game->hangman->win_previous_round = false;
    game->hangman->round_won_display_time = 0; // Initialize display timer

    if (game->text_font) {
        SDL_Color white = {255, 255, 255, 255};
        char letter[2] = {0};
        
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            letter[0] = 'A' + i;
            SDL_Surface* surface = TTF_RenderText_Blended(game->text_font, letter, white);
            if (surface) {
                game->hangman->letter_textures[i] = SDL_CreateTextureFromSurface(game->renderer, surface);
                SDL_FreeSurface(surface);
            } else {
                fprintf(stderr, "ERROR: hard_mode_init: Failed to create surface for letter %c: %s\n", letter[0], TTF_GetError());
            }
        }
        fprintf(stderr, "DEBUG: hard_mode_init: Letter textures created.\n");
    } else {
        fprintf(stderr, "ERROR: hard_mode_init: game->text_font is NULL. Cannot create letter textures. This is critical.\n");
        hard_mode_cleanup(game); // Clean up already allocated hangman data
        game->hangman = NULL; // Ensure it's NULL
        return;
    }
    
    int idx = 0;
    for (int row = 0; row < KEYBOARD_ROWS; row++) {
        for (int col = 0; col < KEYBOARD_COLS; col++) {
            if (idx < ALPHABET_SIZE) {
                game->hangman->letter_rects[idx].x = KEYBOARD_START_X + col * (KEY_WIDTH + KEY_SPACING);
                game->hangman->letter_rects[idx].y = KEYBOARD_START_Y + row * (KEY_HEIGHT + KEY_SPACING);
                game->hangman->letter_rects[idx].w = KEY_WIDTH;
                game->hangman->letter_rects[idx].h = KEY_HEIGHT;
                idx++;
            }
        }
    }
    fprintf(stderr, "DEBUG: hard_mode_init: Keyboard layout setup.\n");
    
    hard_mode_reset(game);
    fprintf(stderr, "DEBUG: hard_mode_init completed successfully.\n");
}

void hard_mode_cleanup(Game* game) {
    fprintf(stderr, "DEBUG: hard_mode_cleanup called.\n");
    if (game == NULL) {
        fprintf(stderr, "DEBUG: hard_mode_cleanup: Game pointer is NULL. Nothing to clean.\n");
        return;
    }
    if (game->hangman) {
        fprintf(stderr, "DEBUG: hard_mode_cleanup: Cleaning up game->hangman data at %p.\n", (void*)game->hangman);
        if (game->hangman->word_list_dynamic) {
            for (int i = 0; i < game->hangman->word_count_dynamic; i++) {
                if (game->hangman->word_list_dynamic[i]) { // Check individual word pointers
                    free(game->hangman->word_list_dynamic[i]);
                }
            }
            free(game->hangman->word_list_dynamic);
            game->hangman->word_list_dynamic = NULL;
            fprintf(stderr, "DEBUG: Freed word_list_dynamic.\n");
        } else {
            fprintf(stderr, "DEBUG: hard_mode_cleanup: word_list_dynamic was NULL.\n");
        }

        for (int i = 0; i < ALPHABET_SIZE; i++) {
            if (game->hangman->letter_textures[i]) {
                SDL_DestroyTexture(game->hangman->letter_textures[i]);
                game->hangman->letter_textures[i] = NULL;
            }
        }
        fprintf(stderr, "DEBUG: Freed letter textures.\n");
        free(game->hangman);
        game->hangman = NULL;
        fprintf(stderr, "DEBUG: Freed game->hangman and set to NULL.\n");
    } else {
        fprintf(stderr, "DEBUG: hard_mode_cleanup: game->hangman was already NULL.\n");
    }
}


// Handle events specific to hard mode
void hard_mode_handle_event(Game* game, SDL_Event* event) { // 'event' is a pointer here
    fprintf(stderr, "DEBUG: hard_mode_handle_event called.\n");
    if (game == NULL || game->hangman == NULL) {
        fprintf(stderr, "ERROR: hard_mode_handle_event: game or game->hangman is NULL. Cannot handle event.\n");
        return;
    }
    switch (event->type) {
        case SDL_KEYDOWN:
            if (event->key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                // This escape handling is now centralized in interface.c
                // game->current_state = MAIN_MENU;
            } else {
                // Process key input for hangman game
                SDL_Keycode key = event->key.keysym.sym;
                // Only process alphabetical keys
                if ((key >= SDLK_a && key <= SDLK_z)) {
                    hard_mode_process_key(game, (char)key);
                }
            }
            break;
            
        case SDL_MOUSEBUTTONDOWN:
            if (event->button.button == SDL_BUTTON_LEFT) {
                // If game is definitively over (lost or overall won), click resets.
                // If a round was just won (game->hangman->win is true), ignore clicks.
                if (game->hangman->game_over) {
                    fprintf(stderr, "DEBUG: hard_mode_handle_event: Game over, click to reset.\n");
                    hard_mode_reset(game);
                } else if (!game->hangman->win) { // Only process clicks if not currently displaying round win
                    // Check if click is on a keyboard key
                    for (int i = 0; i < ALPHABET_SIZE; i++) {
                        SDL_Rect rect = game->hangman->letter_rects[i];
                        // CORRECTED: Use 'event->button.x' and 'event->button.y'
                        if (event->button.x >= rect.x && event->button.x <= rect.x + rect.w &&
                            event->button.y >= rect.y && event->button.y <= rect.y + rect.h) {
                            hard_mode_process_key(game, 'A' + i);
                            break;
                        }
                    }
                }
            }
            break;
    }
}

// Render the hard mode game
void hard_mode_render(Game* game) {
    // fprintf(stderr, "DEBUG: hard_mode_render called.\n"); // Too frequent, might spam
    if (game == NULL || game->hangman == NULL) {
        fprintf(stderr, "ERROR: hard_mode_render: game or game->hangman is NULL. Cannot render hard mode.\n");
        return;
    }

    SDL_SetRenderDrawColor(game->renderer, 50, 50, 150, 255); // A distinct background color for Hard Mode (dark blue)
    SDL_RenderClear(game->renderer);

    // --- Timer Update Logic ---
    // Only update timer if the game is NOT definitively over (lost by guesses/time, or overall won)
    // AND it's not currently displaying a round win message.
    if (!game->hangman->game_over && !game->hangman->win) {
        long elapsed_time_ms = SDL_GetTicks() - game->hangman->start_time_ms;
        game->hangman->time_left_ms = game->hangman->current_round_time_limit_ms - elapsed_time_ms;

        if (game->hangman->time_left_ms <= 0) {
            game->hangman->time_left_ms = 0; // Cap at 0
            game->hangman->game_over = true; // Game over due to timer
            game->hangman->win = false;      // Player loses
            game->hangman->win_previous_round = false; // Mark previous round as a loss
            fprintf(stderr, "DEBUG: hard_mode_render: Time ran out! Game Over.\n");
        }
    }

    // Render the title
    render_text(game->renderer, game->text_font, "HARD MODE", (SDL_Color){255, 255, 0, 255},
                (WIDTH - (strlen("HARD MODE") * FONT_SIZE / 2)) / 2, 50); // Approximate centering

    // Render the timer
    char timer_str[50];
    long seconds_left = game->hangman->time_left_ms / 1000;
    snprintf(timer_str, sizeof(timer_str), "Time: %02ld:%02ld", seconds_left / 60, seconds_left % 60);
    SDL_Color timer_color = {255, 255, 255, 255}; // White
    if (seconds_left <= 10 && !game->hangman->game_over && !game->hangman->win) { // Flash red when low, only if game is active
        timer_color = (SDL_Color){255, 0, 0, 255};
    }
    render_text(game->renderer, game->text_font, timer_str, timer_color,
                WIDTH - 200, 20); // Position at top right

    // Render current word length target
    char length_str[50];
    snprintf(length_str, sizeof(length_str), "Word Length: %d/%d", game->hangman->current_word_length, MAX_GAME_WORD_LENGTH);
    render_text(game->renderer, game->text_font, length_str, (SDL_Color){255, 255, 255, 255},
                20, 20); // Position at top left

    // Render wrong guesses count
    char wrong_guesses_text[50];
    snprintf(wrong_guesses_text, sizeof(wrong_guesses_text), "Wrong Guesses: %d/%d", game->hangman->wrong_guesses, MAX_WRONG_GUESSES);
    render_text(game->renderer, game->text_font, wrong_guesses_text, (SDL_Color){255, 255, 255, 255},
                20, 70); // Below word length

    // --- Conditional Rendering based on game state ---
    if (game->hangman->win && !game->hangman->game_over) {
        // Player just won a round, display message briefly, then transition
        long current_time = SDL_GetTicks();
        long time_since_win = current_time - game->hangman->round_won_display_time;

        if (time_since_win < ROUND_WIN_DISPLAY_DURATION) {
            // Display "WORD GUESSED! NEXT ROUND!" message
            render_text(game->renderer, game->text_font, "WORD GUESSED! NEXT ROUND!", (SDL_Color){0, 255, 0, 255},
                        (WIDTH - (strlen("WORD GUESSED! NEXT ROUND!") * FONT_SIZE / 2)) / 2, (HEIGHT - FONT_SIZE) / 2);
            fprintf(stderr, "DEBUG: Displaying round win message. Time remaining: %ld ms.\n", ROUND_WIN_DISPLAY_DURATION - time_since_win);
        } else {
            // Time is up, transition to next word
            fprintf(stderr, "DEBUG: Round win display duration over. Resetting for next round.\n");
            hard_mode_reset(game); // This will load the next word and reset round state
        }
    } else if (!game->hangman->game_over) {
        // Game is actively playing (not over, and not in round-win display phase)
        render_hangman_image(game->renderer, game->hangman->wrong_guesses, 0, 0, false); 
        
        render_text(game->renderer, game->text_font, game->hangman->displayed_word, (SDL_Color){255, 255, 255, 255},
                    (WIDTH - (strlen(game->hangman->displayed_word) * FONT_SIZE / 2)) / 2, 400);

        render_keyboard(game);

    } else {
        // Game is definitively over (lost by guesses/time, or overall won), render game over messages
        char message[100];
        SDL_Color message_color;

        if (game->hangman->win) { // This means overall game win (set in hard_mode_reset)
            snprintf(message, sizeof(message), "CONGRATULATIONS! YOU'VE GUESSED ALL WORDS!");
            message_color = (SDL_Color){0, 255, 0, 255}; // Green for overall win
            fprintf(stderr, "DEBUG: hard_mode_render: Displaying overall game won message.\n");
        } else { // This means round loss (by time or guesses)
            if (game->hangman->time_left_ms <= 0) {
                snprintf(message, sizeof(message), "TIME'S UP! GAME OVER!");
            } else {
                snprintf(message, sizeof(message), "GAME OVER! Out of guesses!");
            }
            message_color = (SDL_Color){255, 0, 0, 255}; // Red for loss
            fprintf(stderr, "DEBUG: hard_mode_render: Displaying game over message (loss).\n");
        }
        render_text(game->renderer, game->text_font, message, message_color,
                    (WIDTH - (strlen(message) * FONT_SIZE / 2)) / 2, 150);

        // Render "The word was: %s" only if the player lost a round
        if (!game->hangman->win) { // Only show word if lost
            snprintf(message, sizeof(message), "The word was: %s", game->hangman->word);
            render_text(game->renderer, game->text_font, message, (SDL_Color){255, 255, 255, 255},
                        (WIDTH - (strlen(message) * FONT_SIZE / 2)) / 2, 250);
        }

        // Render "Press any key to play again" only for a definitive game over
        render_text(game->renderer, game->text_font, "Press any key to play again", (SDL_Color){255, 255, 255, 255},
                    (WIDTH - (strlen("Press any key to play again") * FONT_SIZE / 2)) / 2, 500);
    }
}
