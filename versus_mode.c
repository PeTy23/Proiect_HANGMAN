#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>

#include "versus_mode.h"
#include "normal_mode.h"
#include "interface.h"

#define WORDLIST_FILENAME "words.txt"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


void versus_mode_init(Game* game) {
    srand(time(NULL));
    game->versus_data = (VersusHangman*)calloc(1, sizeof(VersusHangman));
    if (game->versus_data == NULL) {
        fprintf(stderr, "ERROR: versus_mode_init: Failed to allocate memory for VersusGameData.\n");
        return;
    }

    game->versus_data->overall_game_over_by_time = false;

    memset(&game->versus_data->player1, 0, sizeof(HangmanGame));
    game->versus_data->player1.words_guessed_count = 0;

    if (!normal_mode_load_words_from_file(&game->versus_data->player1, game->current_language)) {
        fprintf(stderr, "ERROR: versus_mode_init: Failed to load words for player 1. Exiting.\n");
        versus_mode_cleanup(game);
        return;
    }
    SDL_Color default_key_color = {255, 255, 255, 255};
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        char key_char[2];
        key_char[0] = 'A' + i;
        key_char[1] = '\0';
        SDL_Surface* text_surface = TTF_RenderText_Blended(game->text_font, key_char, default_key_color);
        if (text_surface) {
            game->versus_data->player1.letter_textures[i] = SDL_CreateTextureFromSurface(game->renderer, text_surface);
            SDL_FreeSurface(text_surface);
        } else {
            fprintf(stderr, "ERROR: versus_mode_init: Failed to create text surface for key %c (P1): %s\n", key_char[0], TTF_GetError());
        }
    }
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        int row = i / 13;
        int col = i % 13;
        game->versus_data->player1.letter_rects[i].x = 180 + col * (40 + 10);
        game->versus_data->player1.letter_rects[i].y = 650 + row * (40 + 10);
        game->versus_data->player1.letter_rects[i].w = 40;
        game->versus_data->player1.letter_rects[i].h = 40;
    }

    memset(&game->versus_data->player2, 0, sizeof(HangmanGame));
    game->versus_data->player2.words_guessed_count = 0;
    game->versus_data->player2.word_list_dynamic = game->versus_data->player1.word_list_dynamic;
    game->versus_data->player2.word_count_dynamic = game->versus_data->player1.word_count_dynamic;

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        char key_char[2];
        key_char[0] = 'A' + i;
        key_char[1] = '\0';
        SDL_Surface* text_surface = TTF_RenderText_Blended(game->text_font, key_char, default_key_color);
        if (text_surface) {
            game->versus_data->player2.letter_textures[i] = SDL_CreateTextureFromSurface(game->renderer, text_surface);
            SDL_FreeSurface(text_surface);
        } else {
            fprintf(stderr, "ERROR: versus_mode_init: Failed to create text surface for key %c (P2): %s\n", key_char[0], TTF_GetError());
        }
    }
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        int row = i / 13;
        int col = i % 13;
        game->versus_data->player2.letter_rects[i].x = 180 + col * (40 + 10);
        game->versus_data->player2.letter_rects[i].y = 650 + row * (40 + 10);
        game->versus_data->player2.letter_rects[i].w = 40;
        game->versus_data->player2.letter_rects[i].h = 40;
    }

    versus_mode_reset(game, true);
}

void versus_mode_cleanup(Game* game) {
    if (game->versus_data) {
        if (game->versus_data->player1.word_list_dynamic) {
            for (int i = 0; i < game->versus_data->player1.word_count_dynamic; i++) {
                free(game->versus_data->player1.word_list_dynamic[i]);
            }
            free(game->versus_data->player1.word_list_dynamic);
            game->versus_data->player1.word_list_dynamic = NULL;
            game->versus_data->player2.word_list_dynamic = NULL;
        }

        for (int i = 0; i < ALPHABET_SIZE; i++) {
            if (game->versus_data->player1.letter_textures[i]) {
                SDL_DestroyTexture(game->versus_data->player1.letter_textures[i]);
                game->versus_data->player1.letter_textures[i] = NULL;
            }
        }
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            if (game->versus_data->player2.letter_textures[i]) {
                SDL_DestroyTexture(game->versus_data->player2.letter_textures[i]);
                game->versus_data->player2.letter_textures[i] = NULL;
            }
        }
        free(game->versus_data);
        game->versus_data = NULL;
    }
}

void versus_mode_reset(Game* game, bool full_game_reset) {
    if (!game->versus_data) {
        fprintf(stderr, "ERROR: versus_mode_reset: VersusGameData pointer is NULL.\n");
        return;
    }

    HangmanGame temp_player1_persistent_data = {0};
    HangmanGame temp_player2_persistent_data = {0};

    // Store persistent data (word list pointers, words_guessed_count, AND time_left_ms)
    // before zeroing out the HangmanGame structs, then restore them.
    if (!full_game_reset) {
        temp_player1_persistent_data.word_list_dynamic = game->versus_data->player1.word_list_dynamic;
        temp_player1_persistent_data.word_count_dynamic = game->versus_data->player1.word_count_dynamic;
        temp_player1_persistent_data.words_guessed_count = game->versus_data->player1.words_guessed_count;
        temp_player1_persistent_data.time_left_ms = game->versus_data->player1.time_left_ms;

        temp_player2_persistent_data.word_list_dynamic = game->versus_data->player2.word_list_dynamic;
        temp_player2_persistent_data.word_count_dynamic = game->versus_data->player2.word_count_dynamic;
        temp_player2_persistent_data.words_guessed_count = game->versus_data->player2.words_guessed_count;
        temp_player2_persistent_data.time_left_ms = game->versus_data->player2.time_left_ms;
    }

    // --- Reset round-specific data for Player 1 ---
    memset(&game->versus_data->player1, 0, sizeof(HangmanGame)); // Clear all round-specific members
    game->versus_data->player1.word_list_dynamic = temp_player1_persistent_data.word_list_dynamic;
    game->versus_data->player1.word_count_dynamic = temp_player1_persistent_data.word_count_dynamic;
    game->versus_data->player1.words_guessed_count = temp_player1_persistent_data.words_guessed_count;
    if (full_game_reset) {
        game->versus_data->player1.time_left_ms = INITIAL_VERSUS_MODE_TIME_SECONDS * 1000;
    } else {
        game->versus_data->player1.time_left_ms = temp_player1_persistent_data.time_left_ms;
    }

    // --- Reset round-specific data for Player 2 ---
    memset(&game->versus_data->player2, 0, sizeof(HangmanGame)); // Clear all round-specific members
    game->versus_data->player2.word_list_dynamic = temp_player2_persistent_data.word_list_dynamic;
    game->versus_data->player2.word_count_dynamic = temp_player2_persistent_data.word_count_dynamic;
    game->versus_data->player2.words_guessed_count = temp_player2_persistent_data.words_guessed_count;
    if (full_game_reset) {
        game->versus_data->player2.time_left_ms = INITIAL_VERSUS_MODE_TIME_SECONDS * 1000;
    } else {
        game->versus_data->player2.time_left_ms = temp_player2_persistent_data.time_left_ms;
    }

    // --- Overall Game Specific Reset Logic ---
    if (full_game_reset) {
        game->versus_data->player1.words_guessed_count = 0;
        game->versus_data->player2.words_guessed_count = 0;

        if (game->versus_data->player1.word_list_dynamic == NULL) {
            if (!normal_mode_load_words_from_file(&game->versus_data->player1, game->current_language)) {
                fprintf(stderr, "ERROR: versus_mode_reset: Failed to re-load words for new game.\n");
                game->current_state = MAIN_MENU;
                return;
            }
            game->versus_data->player2.word_list_dynamic = game->versus_data->player1.word_list_dynamic;
            game->versus_data->player2.word_count_dynamic = game->versus_data->player1.word_count_dynamic;
        }
    }
    game->versus_data->overall_game_over_by_time = false; // This flag now means 'overall game over for any reason'

    // --- OPTION B IMPLEMENTATION: Randomize common_word_length for every new round ---
    // This line is now outside the 'if (full_game_reset)' block,
    // ensuring it's executed every time versus_mode_reset is called.
    game->versus_data->common_word_length = 4 + (rand() % 7);


    // Pick new words for the round (always of the current common_word_length)
    // This is the CRITICAL part for the initial word length mismatch.
    // Ensure this is the FIRST and ONLY place where words are assigned after a reset.
    const char* word_p1 = normal_mode_get_random_word_of_length(&game->versus_data->player1, game->versus_data->common_word_length);
    if (word_p1) {
        strncpy(game->versus_data->player1.word, word_p1, MAX_WORD_LENGTH);
        game->versus_data->player1.word[MAX_WORD_LENGTH] = '\0';
    } else {
        strncpy(game->versus_data->player1.word, "DEFAULT", MAX_WORD_LENGTH);
        game->versus_data->player1.word[MAX_WORD_LENGTH] = '\0';
        fprintf(stderr, "WARNING: versus_mode_reset: No words of length %d for P1 in new round.\n", game->versus_data->common_word_length);
    }
    versus_mode_update_displayed_word(&game->versus_data->player1);

    const char* word_p2 = normal_mode_get_random_word_of_length(&game->versus_data->player2, game->versus_data->common_word_length);
    if (word_p2) {
        strncpy(game->versus_data->player2.word, word_p2, MAX_WORD_LENGTH);
        game->versus_data->player2.word[MAX_WORD_LENGTH] = '\0';
    } else {
        strncpy(game->versus_data->player2.word, "DEFAULT", MAX_WORD_LENGTH);
        game->versus_data->player2.word[MAX_WORD_LENGTH] = '\0';
        fprintf(stderr, "WARNING: versus_mode_reset: No words of length %d for P2 in new round.\n", game->versus_data->common_word_length);
    }
    versus_mode_update_displayed_word(&game->versus_data->player2);

    // Randomly decide who goes first for the new round
    game->versus_data->current_turn = (rand() % 2 == 0) ? PLAYER_1 : PLAYER_2;
    game->versus_data->round_over_display_time = 0; // Reset display timer for next round/game start

    // Set the start time for the first player of the new round
    if (game->versus_data->current_turn == PLAYER_1) {
        game->versus_data->player1.start_time_ms = SDL_GetTicks();
    } else {
        game->versus_data->player2.start_time_ms = SDL_GetTicks();
    }
    fprintf(stderr, "DEBUG: Versus Mode Reset. P1 Words: %d, P2 Words: %d. Common Length: %d. Turn: P%d.\n",
            game->versus_data->player1.words_guessed_count, game->versus_data->player2.words_guessed_count,
            game->versus_data->common_word_length, (game->versus_data->current_turn == PLAYER_1 ? 1 : 2));
}

void versus_mode_handle_event(Game* game, SDL_Event* event) {
    if (!game->versus_data) return;

    bool p1_overall_winner_by_words = (game->versus_data->player1.words_guessed_count >= WORDS_TO_WIN_VERSUS_MODE);
    bool p2_overall_winner_by_words = (game->versus_data->player2.words_guessed_count >= WORDS_TO_WIN_VERSUS_MODE);
    bool game_over_by_time_or_guesses_flag = game->versus_data->overall_game_over_by_time; // This flag covers time AND guesses now

    bool overall_game_over_state = p1_overall_winner_by_words || p2_overall_winner_by_words || game_over_by_time_or_guesses_flag;


    if (overall_game_over_state) {
        if (SDL_GetTicks() - game->versus_data->round_over_display_time >= 3000) {
            if (event->type == SDL_KEYDOWN || event->type == SDL_MOUSEBUTTONDOWN) {
                versus_mode_reset(game, true);
            }
        }
        return;
    }

    bool current_round_just_ended_for_player1 = game->versus_data->player1.game_over;
    bool current_round_just_ended_for_player2 = game->versus_data->player2.game_over;

    if (current_round_just_ended_for_player1 || current_round_just_ended_for_player2) {
        if (SDL_GetTicks() - game->versus_data->round_over_display_time >= 1500) {
            // Call versus_mode_reset(false) for a round reset.
            // This will re-randomize common_word_length and pick new words for BOTH players.
            versus_mode_reset(game, false);
        }
        return;
    }


    switch (event->type) {
        case SDL_KEYDOWN:
            {
                char key = '\0';
                if (event->key.keysym.sym >= SDLK_a && event->key.keysym.sym <= SDLK_z) {
                    key = (char)toupper(event->key.keysym.sym);
                }

                if (event->key.keysym.sym == SDLK_ESCAPE) {
                    game->current_state = MAIN_MENU;
                    return;
                }

                if (key != '\0') {
                    versus_mode_process_key(game, key);
                }
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (event->button.button == SDL_BUTTON_LEFT) {
                for (int i = 0; i < ALPHABET_SIZE; i++) {
                    SDL_Rect rect = game->versus_data->player1.letter_rects[i];
                    if (event->button.x >= rect.x && event->button.x <= rect.x + rect.w &&
                        event->button.y >= rect.y && event->button.y <= rect.y + rect.h) {
                        versus_mode_process_key(game, 'A' + i);
                        break;
                    }
                }
            }
            break;
    }
}

void versus_mode_process_key(Game* game, char key) {
    if (!game || !game->versus_data) {
        fprintf(stderr, "ERROR: versus_mode_process_key: Game or versus_data is NULL.\n");
        return;
    }

    HangmanGame* active_player = NULL;
    HangmanGame* inactive_player = NULL; // Pointer to the opponent
    if (game->versus_data->current_turn == PLAYER_1) {
        active_player = &game->versus_data->player1;
        inactive_player = &game->versus_data->player2;
    } else {
        active_player = &game->versus_data->player2;
        inactive_player = &game->versus_data->player1;
    }

    int index = key - 'A';
    if (index < 0 || index >= ALPHABET_SIZE) {
        fprintf(stderr, "DEBUG: versus_mode_process_key: Non-alphabetical key '%c' ignored.\n", key);
        return;
    }

    bool was_already_marked_as_guessed = active_player->guessed_letters[index];

    bool found_in_word = false;
    for (size_t i = 0; i < strlen(active_player->word); i++) {
        if (active_player->word[i] == key) {
            found_in_word = true;
            break;
        }
    }

    // --- Handle already guessed letters (correct or incorrect) ---
    if (was_already_marked_as_guessed) {
        if (found_in_word) {
            fprintf(stderr, "DEBUG: Player %d: Letter '%c' already correctly guessed. Ignoring input.\n",
                    (game->versus_data->current_turn == PLAYER_1 ? 1 : 2), key);
            return;
        } else {
            // It was already guessed AND it's wrong (repeated wrong guess).
            active_player->wrong_guesses++; // Add 1 more wrong guess.
            fprintf(stderr, "DEBUG: Player %d: Repeated incorrect guess '%c'. Added 1 wrong guess. Total: %d.\n",
                    (game->versus_data->current_turn == PLAYER_1 ? 1 : 2), key, active_player->wrong_guesses);
            // After this, flow will proceed to check round/game end and then turn switch.
        }
    } else {
        // This is a NEW guess (not previously marked).
        active_player->guessed_letters[index] = true;

        if (!found_in_word) {
            active_player->wrong_guesses++; // New incorrect guess.
            fprintf(stderr, "DEBUG: Player %d: New incorrect guess '%c'. Wrong guesses: %d\n",
                    (game->versus_data->current_turn == PLAYER_1 ? 1 : 2), key, active_player->wrong_guesses);
        } else {
            active_player->time_left_ms += (TIME_BONUS_GUESS_SECONDS * 1000); // New correct guess.
            fprintf(stderr, "DEBUG: Player %d: Correct guess '%c'. Time bonus added. New time: %ld ms\n",
                    (game->versus_data->current_turn == PLAYER_1 ? 1 : 2), key, active_player->time_left_ms);
        }
    }

    versus_mode_update_displayed_word(active_player);

    bool word_guessed_completely = true;
    for (size_t i = 0; i < strlen(active_player->word); i++) {
        char c = active_player->word[i];
        if (c >= 'A' && c <= 'Z' && !active_player->guessed_letters[c - 'A']) {
            word_guessed_completely = false;
            break;
        }
    }

    // --- Round End / Overall Game End Conditions ---
    if (word_guessed_completely) {
        active_player->win = true; // Player won this round
        active_player->words_guessed_count++; // Increment overall word count
        fprintf(stderr, "DEBUG: Player %d guessed the word! Word count: %d/%d.\n",
                (game->versus_data->current_turn == PLAYER_1 ? 1 : 2),
                active_player->words_guessed_count, WORDS_TO_WIN_VERSUS_MODE);

        // Apply bonus guesses for guessing the word
        active_player->wrong_guesses -= GUESS_BONUS_WORD_GUESSED;
        if (active_player->wrong_guesses < 0) {
            active_player->wrong_guesses = 0;
        }
        fprintf(stderr, "DEBUG: Player %d receives %d guess bonus for guessing word. New wrong guesses: %d.\n",
                (game->versus_data->current_turn == PLAYER_1 ? 1 : 2),
                GUESS_BONUS_WORD_GUESSED, active_player->wrong_guesses);

        // --- CONSOLATION PRIZE FOR OPPONENT (INACTIVE PLAYER) ---
        // If the active player guessed the word, the inactive player effectively lost this round.
        inactive_player->wrong_guesses -= GUESS_BONUS_ROUND_LOST;
        if (inactive_player->wrong_guesses < 0) {
            inactive_player->wrong_guesses = 0;
        }
        fprintf(stderr, "DEBUG: Player %d (opponent) receives %d consolation bonus. New wrong guesses: %d.\n",
                (game->versus_data->current_turn == PLAYER_1 ? 2 : 1),
                GUESS_BONUS_ROUND_LOST, inactive_player->wrong_guesses);


        // If guessing this word makes them an overall winner, set flags for game end.
        if (active_player->words_guessed_count >= WORDS_TO_WIN_VERSUS_MODE) {
            game->versus_data->round_over_display_time = SDL_GetTicks(); // Will trigger overall game end message
        } else {
            // This is a "round win" that will trigger a full round reset in handle_event
            // (which will get new words for both players and randomize common_word_length).
            // Do NOT generate a new word here for active_player. Let versus_mode_reset handle both.
            game->versus_data->player1.game_over = true; // Temporary flags to trigger round transition message
            game->versus_data->player2.game_over = true; // These will be cleared by versus_mode_reset(false)
            game->versus_data->round_over_display_time = SDL_GetTicks();
        }

    } else if (active_player->wrong_guesses >= MAX_WRONG_GUESSES) {
        active_player->win = false; // Player lost this word/round
        fprintf(stderr, "DEBUG: Player %d ran out of guesses! This player loses the game.\n",
                (game->versus_data->current_turn == PLAYER_1 ? 1 : 2));

        // This player ran out of guesses, so the overall game ends.
        game->versus_data->overall_game_over_by_time = true; // This flag now means 'overall game over for any reason'
        game->versus_data->round_over_display_time = SDL_GetTicks(); // Timestamp for end-game display
        // NO CONSOLATION PRIZE HERE: Player lost the entire game by their own fault.
        // No turn switch if the game is definitively over.
    } else {
        // Game is not over for the active player, decide turn switch or continue
        if (!found_in_word) { // Turn switches ONLY on an incorrect guess (including repeated wrong guesses)
            long current_time = SDL_GetTicks();
            active_player->time_left_ms -= (current_time - active_player->start_time_ms);
            if (active_player->time_left_ms < 0) active_player->time_left_ms = 0;

            fprintf(stderr, "DEBUG: Player %d's turn ends (incorrect guess). Remaining time: %ld ms. Switching to Player %d.\n",
                    (game->versus_data->current_turn == PLAYER_1 ? 1 : 2), active_player->time_left_ms,
                    (game->versus_data->current_turn == PLAYER_1 ? 2 : 1));

            game->versus_data->current_turn = (game->versus_data->current_turn == PLAYER_1) ? PLAYER_2 : PLAYER_1;

            if (game->versus_data->current_turn == PLAYER_1) {
                game->versus_data->player1.start_time_ms = SDL_GetTicks();
            } else {
                game->versus_data->player2.start_time_ms = SDL_GetTicks();
            }
        } else { // Correct guess, current player's turn continues on the SAME word (or the new word generated if word was guessed)
            active_player->start_time_ms = SDL_GetTicks();
            fprintf(stderr, "DEBUG: Player %d's turn continues (correct guess '%c'). Timer reset for current turn segment.\n",
                    (game->versus_data->current_turn == PLAYER_1 ? 1 : 2), key);
        }
    }
}


void versus_mode_update_displayed_word(HangmanGame* hangman) {
    if (!hangman) return;

    int display_idx = 0;
    for (size_t i = 0; i < strlen(hangman->word); i++) {
        char c = hangman->word[i];
        if (hangman->guessed_letters[c - 'A']) {
            hangman->displayed_word[display_idx++] = c;
        } else {
            hangman->displayed_word[display_idx++] = '_';
        }
        if (i < strlen(hangman->word) - 1) {
            hangman->displayed_word[display_idx++] = ' ';
        }
    }
    hangman->displayed_word[display_idx] = '\0';
}


void versus_mode_render(Game* game) {
    if (!game->versus_data) return;

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color red = {255, 0, 0, 255};
    SDL_Color green = {0, 255, 0, 255};
    SDL_Color yellow = {255, 255, 0, 255};

    SDL_SetRenderDrawColor(game->renderer, 100, 50, 100, 255); // Purple-ish background
    SDL_RenderClear(game->renderer);

    int displayed_word_y = 400; // Y-position for the displayed underscores/words
    const int DEFAULT_GALLOWS_VERTICAL_POST_X = 150; // Reference X for gallows' main post in render_hangman_image


    // --- Player 1 Display (Left Side) ---
    int p1_gallows_target_center_x = (WIDTH / 4); // Center of the left quarter of the screen
    int p1_gallows_x_offset = p1_gallows_target_center_x - DEFAULT_GALLOWS_VERTICAL_POST_X;
    render_hangman_image(game->renderer, game->versus_data->player1.wrong_guesses, p1_gallows_x_offset, 0, false); // Render P1's hangman (not mirrored)

    render_text(game->renderer, game->text_font, "Player 1", (game->versus_data->current_turn == PLAYER_1) ? yellow : white,
                (WIDTH / 4) - (strlen("Player 1") * FONT_SIZE / 4), 50); // P1 Name

    // Display words guessed count for Player 1
    char p1_words_guessed_str[50];
    snprintf(p1_words_guessed_str, sizeof(p1_words_guessed_str), "Words: %d/%d",
             game->versus_data->player1.words_guessed_count, WORDS_TO_WIN_VERSUS_MODE);
    render_text(game->renderer, game->text_font, p1_words_guessed_str, white,
                (WIDTH / 4) - (strlen(p1_words_guessed_str) * FONT_SIZE / 4), 150); // Position below timer/name

    render_text(game->renderer, game->text_font, game->versus_data->player1.displayed_word, white,
                (WIDTH / 4) - (strlen(game->versus_data->player1.displayed_word) * FONT_SIZE / 4), displayed_word_y); // P1's word progress
    char p1_guesses_str[50];
    snprintf(p1_guesses_str, sizeof(p1_guesses_str), "Wrong Guesses: %d/%d", game->versus_data->player1.wrong_guesses, MAX_WRONG_GUESSES);
    render_text(game->renderer, game->text_font, p1_guesses_str, white,
                (WIDTH / 4) - (strlen(p1_guesses_str) * FONT_SIZE / 4), 550); // P1's wrong guesses


    // --- Player 2 Display (Right Side) ---
    int p2_gallows_target_center_x = (WIDTH * 3 / 4); // Center of the right quarter of the screen
    int p2_gallows_x_offset = p2_gallows_target_center_x - DEFAULT_GALLOWS_VERTICAL_POST_X;
    render_hangman_image(game->renderer, game->versus_data->player2.wrong_guesses, p2_gallows_x_offset, 0, true); // Render P2's hangman (mirrored)

    render_text(game->renderer, game->text_font, "Player 2", (game->versus_data->current_turn == PLAYER_2) ? yellow : white,
                (WIDTH * 3 / 4) - (strlen("Player 2") * FONT_SIZE / 4), 50); // P2 Name

    // Display words guessed count for Player 2
    char p2_words_guessed_str[50];
    snprintf(p2_words_guessed_str, sizeof(p2_words_guessed_str), "Words: %d/%d",
             game->versus_data->player2.words_guessed_count, WORDS_TO_WIN_VERSUS_MODE);
    render_text(game->renderer, game->text_font, p2_words_guessed_str, white,
                (WIDTH * 3 / 4) - (strlen(p2_words_guessed_str) * FONT_SIZE / 4), 150); // Position below timer/name

    render_text(game->renderer, game->text_font, game->versus_data->player2.displayed_word, white,
                (WIDTH * 3 / 4) - (strlen(game->versus_data->player2.displayed_word) * FONT_SIZE / 4), displayed_word_y); // P2's word progress
    char p2_guesses_str[50];
    snprintf(p2_guesses_str, sizeof(p2_guesses_str), "Wrong Guesses: %d/%d", game->versus_data->player2.wrong_guesses, MAX_WRONG_GUESSES);
    render_text(game->renderer, game->text_font, p2_guesses_str, white,
                (WIDTH * 3 / 4) - (strlen(p2_guesses_str) * FONT_SIZE / 4), 550); // P2's wrong guesses


    // --- Timer Update Logic ---
    HangmanGame* player1_game = &game->versus_data->player1;
    HangmanGame* player2_game = &game->versus_data->player2;

    // Determine overall game winner/loser state
    bool p1_overall_winner_by_words = (player1_game->words_guessed_count >= WORDS_TO_WIN_VERSUS_MODE);
    bool p2_overall_winner_by_words = (player2_game->words_guessed_count >= WORDS_TO_WIN_VERSUS_MODE);
    bool overall_game_over_by_time_or_guesses_flag = game->versus_data->overall_game_over_by_time; // This flag covers time AND guesses now

    // Overall game is over if any of these conditions are met
    bool overall_game_active_for_timers = !p1_overall_winner_by_words && !p2_overall_winner_by_words && !overall_game_over_by_time_or_guesses_flag;
    bool current_round_active_for_timers = !player1_game->game_over && !player2_game->game_over; // game_over here means round end

    if (overall_game_active_for_timers && current_round_active_for_timers) {
        long current_time = SDL_GetTicks();
        if (game->versus_data->current_turn == PLAYER_1) {
            long elapsed_this_turn = current_time - player1_game->start_time_ms;
            player1_game->time_left_ms -= elapsed_this_turn;
            player1_game->start_time_ms = current_time;

            if (player1_game->time_left_ms <= 0) {
                player1_game->time_left_ms = 0;
                player1_game->game_over = true; // Round end for P1 (by time)
                player1_game->win = false; // Indicates round loss
                game->versus_data->overall_game_over_by_time = true; // Overall game over by time
                game->versus_data->round_over_display_time = SDL_GetTicks(); // Timestamp for end-game display
            }
        } else { // Current turn is Player 2
            long elapsed_this_turn = current_time - player2_game->start_time_ms;
            player2_game->time_left_ms -= elapsed_this_turn;
            player2_game->start_time_ms = current_time;

            if (player2_game->time_left_ms <= 0) {
                player2_game->time_left_ms = 0;
                player2_game->game_over = true; // Round end for P2 (by time)
                player2_game->win = false; // Indicates round loss
                game->versus_data->overall_game_over_by_time = true; // Overall game over by time
                game->versus_data->round_over_display_time = SDL_GetTicks(); // Timestamp for end-game display
            }
        }
    }


    // --- Render Timers ---
    char p1_timer_str[50];
    long p1_seconds_left = player1_game->time_left_ms / 1000;
    snprintf(p1_timer_str, sizeof(p1_timer_str), "Time: %02ld:%02ld", p1_seconds_left / 60, p1_seconds_left % 60);
    // Timer color: yellow if current turn & game active, red if low & current turn, white otherwise
    SDL_Color p1_timer_color = (overall_game_active_for_timers && game->versus_data->current_turn == PLAYER_1) ? yellow : white;
    if (overall_game_active_for_timers && game->versus_data->current_turn == PLAYER_1 && p1_seconds_left <= 10) {
        p1_timer_color = red;
    }
    render_text(game->renderer, game->text_font, p1_timer_str, p1_timer_color,
                (WIDTH / 4) - (strlen(p1_timer_str) * FONT_SIZE / 4), 100);


    char p2_timer_str[50];
    long p2_seconds_left = player2_game->time_left_ms / 1000;
    snprintf(p2_timer_str, sizeof(p2_timer_str), "Time: %02ld:%02ld", p2_seconds_left / 60, p2_seconds_left % 60);
    // Timer color: yellow if current turn & game active, red if low & current turn, white otherwise
    SDL_Color p2_timer_color = (overall_game_active_for_timers && game->versus_data->current_turn == PLAYER_2) ? yellow : white;
    if (overall_game_active_for_timers && game->versus_data->current_turn == PLAYER_2 && p2_seconds_left <= 10) {
        p2_timer_color = red;
    }
    render_text(game->renderer, game->text_font, p2_timer_str, p2_timer_color,
                (WIDTH * 3 / 4) - (strlen(p2_timer_str) * FONT_SIZE / 4), 100);


    // --- Render common keyboard at the bottom ---
    // As per user preference, the keyboard is NOT rendered in Versus Mode.


    // --- Render game over/win messages (Overall Game or Round End) ---
    char message[150];
    SDL_Color message_color;
    bool display_message_overlay = false; // Control whether to display a message OVERLAY

    // Determine the *final* game state for message display
    if (p1_overall_winner_by_words) {
        snprintf(message, sizeof(message), "PLAYER 1 WINS THE GAME! (%d Words)", WORDS_TO_WIN_VERSUS_MODE);
        message_color = green;
        display_message_overlay = true;
    } else if (p2_overall_winner_by_words) {
        snprintf(message, sizeof(message), "PLAYER 2 WINS THE GAME! (%d Words)", WORDS_TO_WIN_VERSUS_MODE);
        message_color = green;
        display_message_overlay = true;
    } else if (overall_game_over_by_time_or_guesses_flag) { // Game over because someone ran out of time OR guesses
        if (player1_game->time_left_ms <= 0 || player1_game->wrong_guesses >= MAX_WRONG_GUESSES) { // P1 ran out of time or guesses
            snprintf(message, sizeof(message), "PLAYER 1 LOST! PLAYER 2 WINS!");
        } else { // P2 ran out of time or guesses
            snprintf(message, sizeof(message), "PLAYER 2 LOST! PLAYER 1 WINS!");
        }
        message_color = green; // Time-out/guess-out is a win for the opponent
        display_message_overlay = true;
    }
    // Then check for round end if no overall winner yet
    else if (player1_game->game_over || player2_game->game_over) { // game_over here means current round is over
        if (player1_game->win) { // Player 1 won this round by guessing word
            snprintf(message, sizeof(message), "PLAYER 1 GUESSED THE WORD! +%d Guesses!", GUESS_BONUS_WORD_GUESSED);
            message_color = green;
        } else if (player2_game->win) { // Player 2 won this round by guessing word
            snprintf(message, sizeof(message), "PLAYER 2 GUESSED THE WORD! +%d Guesses!", GUESS_BONUS_WORD_GUESSED);
            message_color = green;
        } else { // A player lost the round by wrong guesses/time, but no overall winner yet
            if (player1_game->wrong_guesses >= MAX_WRONG_GUESSES || player1_game->time_left_ms <= 0) { // P1 ran out of guesses/time
                snprintf(message, sizeof(message), "PLAYER 1 LOST ROUND! Word was: %s", player1_game->word);
            } else if (player2_game->wrong_guesses >= MAX_WRONG_GUESSES || player2_game->time_left_ms <= 0) { // P2 ran out of guesses/time
                snprintf(message, sizeof(message), "PLAYER 2 LOST ROUND! Word was: %s", player2_game->word);
            } else { // Fallback for unexpected state
                snprintf(message, sizeof(message), "ROUND OVER!");
            }
            message_color = red; // Round lost message is red
        }
        display_message_overlay = true;
    }

    // --- Draw the transparent overlay and final messages ---
    if (display_message_overlay) {
        // Set blend mode for transparency
        SDL_SetRenderDrawBlendMode(game->renderer, SDL_BLENDMODE_BLEND);

        // Draw a semi-transparent black rectangle over the entire screen
        SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 150); // Black with 150 alpha (out of 255)
        SDL_Rect full_screen_rect = {0, 0, WIDTH, HEIGHT};
        SDL_RenderFillRect(game->renderer, &full_screen_rect);

        // Reset blend mode (optional, but good practice if other elements need default blending)
        SDL_SetRenderDrawBlendMode(game->renderer, SDL_BLENDMODE_NONE);

        // Render the message text on top of the overlay
        int message_width;
        TTF_SizeText(game->text_font, message, &message_width, NULL);
        render_text(game->renderer, game->text_font, message, message_color, (WIDTH - message_width) / 2, HEIGHT / 2 - 50);

        // Render "Press any key..." message
        if (overall_game_over_by_time_or_guesses_flag || p1_overall_winner_by_words || p2_overall_winner_by_words) {
            render_text(game->renderer, game->text_font, "Press any key to play again (new game)", white,
                        (WIDTH - (strlen("Press any key to play again (new game)") * FONT_SIZE / 2)) / 2, HEIGHT - 100);
        } else { // Round end, but not overall game end
            render_text(game->renderer, game->text_font, "Next Round in...", white,
                        (WIDTH - (strlen("Next Round in...") * FONT_SIZE / 2)) / 2, HEIGHT - 100);
        }
    }
}