#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h> 
#include <errno.h> 
#include "normal_mode.h"
#include "interface.h" 


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif



bool normal_mode_load_words_from_file(HangmanGame* hangman, GameLanguage lang) {
    if (hangman == NULL) {
        fprintf(stderr, "ERROR: normal_mode_load_words_from_file: HangmanGame pointer is NULL.\n");
        return false;
    }
    const char* filename = NULL;
    if (lang == LANG_ROMANIAN) {
        filename = "words_ro.txt";
    } else {
        filename = "words_en.txt";
    }
    fprintf(stderr, "DEBUG: Loading words from: %s for language %d.\n", filename, lang);

    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "error at opening words.txt\n");
        if (hangman->word_list_dynamic) {
            for (int i = 0; i < hangman->word_count_dynamic; i++) {
                if (hangman->word_list_dynamic[i]) free(hangman->word_list_dynamic[i]);     //se elibereaza fiecare cuvant din array
            }
            free(hangman->word_list_dynamic);
            hangman->word_list_dynamic = NULL;
        }
        hangman->word_count_dynamic = 0;
        return false;
    }

    int count = 0;
    char buffer[MAX_WORD_LENGTH + 2]; //pentru null si \n 
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        char* word = strtok(buffer, "\n"); //numar cuvintele
        if (word && strlen(word) > 0) {
            count++;
        }
    }
    rewind(file);

    hangman->word_count_dynamic = count;
    if (count == 0) {
        fprintf(stderr, "error at loading words from file \n");
        fclose(file);
        return false;
    }

    hangman->word_list_dynamic = (char**)malloc(count * sizeof(char*));
    if (!hangman->word_list_dynamic) {
        fprintf(stderr, "error: at word_list_dynamic memory \n");
        fclose(file);
        return false;
    }

    int i = 0;
    while (fgets(buffer, sizeof(buffer), file) != NULL && i < count) {
        char* word = strtok(buffer, "\n");
        if (word && strlen(word) > 0) {
            hangman->word_list_dynamic[i] = (char*)malloc(strlen(word) + 1); //populez array-ul cu cuvintele din .txt
            if (!hangman->word_list_dynamic[i]) {
                fprintf(stderr, "error at populating array");
                for (int j = 0; j < i; j++) {
                    if (hangman->word_list_dynamic[j]) free(hangman->word_list_dynamic[j]); //daca da fail dau free la tot
                }
                free(hangman->word_list_dynamic);
                hangman->word_list_dynamic = NULL;
                hangman->word_count_dynamic = 0;
                fclose(file);
                return false;
            }
            strcpy(hangman->word_list_dynamic[i], word); //pun cuvintele in array
            for (int k = 0; k < strlen(hangman->word_list_dynamic[i]); k++) {
                if (hangman->word_list_dynamic[i][k] >= 'a' && hangman->word_list_dynamic[i][k] <= 'z') {
                    hangman->word_list_dynamic[i][k] = hangman->word_list_dynamic[i][k] - 32; //transform fiecare litera in litera mare
                }
            }
            i++;
        }
    }

    fclose(file);
    return true;
}

char* normal_mode_get_random_word(HangmanGame* hangman) {
    if (hangman == NULL || hangman->word_list_dynamic == NULL) {
        fprintf(stderr, "error at normal_mode_get_random_word\n");
        return "err";
    }
    int random_nr = rand() % hangman->word_count_dynamic;
    return hangman->word_list_dynamic[random_nr];
}

const char* normal_mode_get_random_word_of_length(HangmanGame* hangman, int length) {
    if (hangman == NULL || hangman->word_count_dynamic == 0 || hangman->word_list_dynamic == NULL) {
        fprintf(stderr, "ERROR: normal_mode_get_random_word_of_length: Word list not loaded or empty.\n");
        return NULL;
    }

    char* suitable_words[hangman->word_count_dynamic];
    int suitable_count = 0;
    for (int i = 0; i < hangman->word_count_dynamic; i++) {
        if (strlen(hangman->word_list_dynamic[i]) == length) {
            suitable_words[suitable_count++] = hangman->word_list_dynamic[i];
        }
    }

    if (suitable_count == 0) {
        fprintf(stderr, "WARNING: normal_mode_get_random_word_of_length: No words found of length %d. Returning random word of any length.\n", length);
        if (hangman->word_count_dynamic > 0) {
            return hangman->word_list_dynamic[rand() % hangman->word_count_dynamic];
        }
        fprintf(stderr, "ERROR: normal_mode_get_random_word_of_length: No words available at all.\n");
        return "DEFAULT"; 
    }

    return suitable_words[rand() % suitable_count];
}



void normal_mode_process_key(Game* game, char key) {
    if (game == NULL || game->hangman == NULL) {
        fprintf(stderr, "error at normal_mode_process_key\n");
        return;
    }
    if (game->hangman->game_over) {
        normal_mode_reset(game);
        return;
    }
    
    if (key >= 'a' && key <= 'z') {
        key = key - 32;
    }

    if (key >= 'A' && key <= 'Z') {
        int index = key - 'A';
        
        if (game->hangman->guessed_letters[index]) { //daca litera a fost gasita
            return;
        }
        
        game->hangman->guessed_letters[index] = true; //marcheaza litera ca fiind gasita
        
        bool found = false;
        for (int i = 0; i < strlen(game->hangman->word); i++) {
            if (game->hangman->word[i] == key) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            game->hangman->wrong_guesses++;
        }
        
        normal_mode_update_displayed_word(game);
    }
}

void normal_mode_update_displayed_word(Game* game) {
    if (game == NULL || game->hangman == NULL) {
        fprintf(stderr, "error at normal mode displayed word\n");
        return;
    }
    int word_len = strlen(game->hangman->word);
    char* display = game->hangman->displayed_word;  
    
    display[0] = '\0';
    
    for (size_t i = 0; i < word_len; i++) {
        char c = game->hangman->word[i];
        int idx = c - 'A';
        
        if (game->hangman->guessed_letters[idx]) {
            char temp[4] = {0};
            sprintf(temp, "%c ", c);
            strcat(display, temp);
        } else {
            strcat(display, "_ ");
        }
    }
    
    bool all_guessed = true;
    for (size_t i = 0; i < word_len; i++) {
        char c = game->hangman->word[i];
        int idx = c - 'A';
        if (!game->hangman->guessed_letters[idx]) {
            all_guessed = false;
            break;
        }
    }
    
    if (all_guessed) {
        game->hangman->game_over = true;
        game->hangman->win = true;
    } else if (game->hangman->wrong_guesses >= MAX_WRONG_GUESSES) {
        game->hangman->game_over = true;
        game->hangman->win = false;
    }
}

void normal_mode_reset(Game* game) {
    if (game == NULL || game->hangman == NULL) {
        fprintf(stderr, "error at normal mode reset\n");
        return;
    }
    const char* chosen_word = normal_mode_get_random_word(game->hangman);
    if (strcmp(chosen_word, "err") == 0) {
        fprintf(stderr, "error at chosen word\n");
        game->hangman->game_over = true; // e game over pe true si win pe false
        game->hangman->win = false;
        return;
    }
    strncpy(game->hangman->word, chosen_word, MAX_WORD_LENGTH);
    game->hangman->word[MAX_WORD_LENGTH] = '\0'; // in hangman se copiaza cuvantul random
    
    memset(game->hangman->guessed_letters, 0, sizeof(game->hangman->guessed_letters)); //pune toate guessed_letters pe 0
    game->hangman->wrong_guesses = 0;
    game->hangman->game_over = false;
    game->hangman->win = false;
    
    normal_mode_update_displayed_word(game);
}

void normal_mode_init(Game* game) {
    if (game == NULL) {
        fprintf(stderr, "ERROR: normal_mode_init: Game pointer is NULL. Aborting initialization.\n");
        return;
    }
    if (game->hangman != NULL) {
        normal_mode_cleanup(game);
    }

    game->hangman = malloc(sizeof(HangmanGame));
    if (!game->hangman) {
        fprintf(stderr, "ERROR: normal_mode_init: Failed to allocate memory for Hangman game: %s\n", strerror(errno));
        game->hangman = NULL;
        return; 
    }
    
    game->hangman->word_list_dynamic = NULL;
    game->hangman->word_count_dynamic = 0;

    srand(time(NULL));
    
    if (!normal_mode_load_words_from_file(game->hangman, game->current_language)) {
        fprintf(stderr, "ERROR: normal_mode_init: Failed to load words from file.\n");
        return; 
    }

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
                fprintf(stderr, "ERROR: normal_mode_init: Failed to create surface for letter %c: %s\n", letter[0], TTF_GetError());
            }
        }
    } else {
        fprintf(stderr, "ERROR: normal_mode_init: game->text_font is NULL. Cannot create letter textures. This is critical.\n");
        normal_mode_cleanup(game); 
        game->hangman = NULL;
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
    
    normal_mode_reset(game);
}

void normal_mode_cleanup(Game* game) {
    if (game == NULL) {
        return;
    }
    if (game->hangman) {
        if (game->hangman->word_list_dynamic) {
            for (int i = 0; i < game->hangman->word_count_dynamic; i++) {
                if (game->hangman->word_list_dynamic[i]) { 
                    free(game->hangman->word_list_dynamic[i]);
                }
            }
            free(game->hangman->word_list_dynamic);
            game->hangman->word_list_dynamic = NULL;
        }

        for (int i = 0; i < ALPHABET_SIZE; i++) {
            if (game->hangman->letter_textures[i]) {
                SDL_DestroyTexture(game->hangman->letter_textures[i]);
                game->hangman->letter_textures[i] = NULL; 
            }
        }
        free(game->hangman);
        game->hangman = NULL;
    }
}

void normal_mode_handle_event(Game* game, SDL_Event* event) {
    if (game == NULL || game->hangman == NULL) {
        fprintf(stderr, "ERROR: normal_mode_handle_event: game or game->hangman is NULL. Cannot handle event.\n");
        return;
    }
    switch (event->type) {
        case SDL_KEYDOWN:
            if (event->key.keysym.sym == SDLK_ESCAPE) { 
            } else {
                SDL_Keycode key = event->key.keysym.sym;
                if (key >= SDLK_a && key <= SDLK_z) {
                    normal_mode_process_key(game, (char)key);
                }
            }
            break;
            
        case SDL_MOUSEBUTTONDOWN:
            if (event->button.button == SDL_BUTTON_LEFT) {
                if (!game->hangman->game_over) {
                    for (int i = 0; i < ALPHABET_SIZE; i++) {
                        SDL_Rect rect = game->hangman->letter_rects[i];
                        if (event->button.x >= rect.x && event->button.x <= rect.x + rect.w &&
                            event->button.y >= rect.y && event->button.y <= rect.y + rect.h) {
                            normal_mode_process_key(game, 'A' + i);
                            break;
                        }
                    }
                } else {
                    normal_mode_reset(game);
                }
            }
            break;
    }
}

void normal_mode_render(Game* game) {
    if (game == NULL || game->hangman == NULL) {
        fprintf(stderr, "ERROR: normal_mode_render: game or game->hangman is NULL. Cannot render normal mode.\n");
        return;
    }
    SDL_SetRenderDrawColor(game->renderer, 30, 30, 30, 255);
    SDL_RenderClear(game->renderer);

    SDL_Color yellow = {255, 255, 0, 255};
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color green = {0, 255, 0, 255};
    SDL_Color red = {255, 0, 0, 255};
    char text_buffer[100];

    render_text(game->renderer, game->text_font, "NORMAL MODE", yellow,
                (WIDTH - (strlen("NORMAL MODE") * FONT_SIZE / 2)) / 2, 50);

    render_hangman_image(game->renderer, game->hangman->wrong_guesses, 0, 0, false); 
    
    render_text(game->renderer, game->text_font, game->hangman->displayed_word, white,
                (WIDTH - (strlen(game->hangman->displayed_word) * FONT_SIZE / 2)) / 2 + 60, 500);
    
    if (game->hangman->game_over) {
        SDL_Color message_color = game->hangman->win ? green : red;
        
        const char* message = game->hangman->win ? "YOU WIN!" : "GAME OVER!";
        render_text(game->renderer, game->text_font, message, message_color,
                    (WIDTH - (strlen(message) * FONT_SIZE / 2)) / 2, 150);

        if (!game->hangman->win) {
            snprintf(text_buffer, sizeof(text_buffer), "The word was: %s", game->hangman->word);
            render_text(game->renderer, game->text_font, text_buffer, white,
                        (WIDTH - (strlen(text_buffer) * FONT_SIZE / 2)) / 2 + 40, 200);
        }
        
        render_text(game->renderer, game->text_font, "Press click to play again", white,
                    (WIDTH - (strlen("Press click to play again") * FONT_SIZE / 2)) / 2 + 50, 700);

    } else {
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            SDL_Rect rect = game->hangman->letter_rects[i];
            SDL_Color key_color = {100, 100, 100, 255}; 
            if (game->hangman->guessed_letters[i]) {
               
                bool in_word = false;
                for (size_t j = 0; j < strlen(game->hangman->word); j++) {
                    if (game->hangman->word[j] == ('A' + i)) {
                        in_word = true;
                        break;
                    }
                }
                key_color = in_word ? green : red;
            }
            SDL_SetRenderDrawColor(game->renderer, key_color.r, key_color.g, key_color.b, key_color.a);
            SDL_RenderFillRect(game->renderer, &rect);

            if (game->hangman->letter_textures[i]) {
                int text_w, text_h;
                SDL_QueryTexture(game->hangman->letter_textures[i], NULL, NULL, &text_w, &text_h);
                SDL_Rect text_rect = {rect.x + (rect.w - text_w) / 2, rect.y + (rect.h - text_h) / 2, text_w, text_h};
                SDL_RenderCopy(game->renderer, game->hangman->letter_textures[i], NULL, &text_rect);
            }
        }
    }
}