#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>   
#include <errno.h>  

#include "interface.h"
#include "normal_mode.h" 
#include "hard_mode.h"   
#include "versus_mode.h"
#define WINDOW_TITLE "HANGMAN"

#define IMAGE_FLAGS IMG_INIT_PNG

void render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Color color, int x, int y) {
    if (!font) {
        fprintf(stderr, "Error at loading font\n");
        return;
    }
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) {
        fprintf(stderr, "Error at creating text surface\n");
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface); //din surface care e pe CPU face in texture care e pe GPU(cred)
    if (!texture) {
        fprintf(stderr, "Error at creating text texture\n");
        SDL_FreeSurface(surface);
        return;
    }
    SDL_Rect text_rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &text_rect);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void render_hangman_image(SDL_Renderer* renderer, int wrong_guesses, int x_offset, int y_offset, bool mirrored) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); 
    int initial_gallows_x = 150;
    int gallows_top = 200 + y_offset;
    int gallows_bottom = 350 + y_offset;
    int base_left_x;
    int base_right_x;
    int vertical_post_x;
    int horizontal_beam_start_x;
    int horizontal_beam_end_x;
    int rope_x;
    int current_gallows_base_x = initial_gallows_x + x_offset;

    if (mirrored) {
        base_left_x = current_gallows_base_x + 50;
        base_right_x = current_gallows_base_x - 50;
        vertical_post_x = current_gallows_base_x;
        horizontal_beam_start_x = current_gallows_base_x;
        horizontal_beam_end_x = current_gallows_base_x - 100; 
        rope_x = horizontal_beam_end_x; 
    } else {
        base_left_x = current_gallows_base_x - 50;
        base_right_x = current_gallows_base_x + 50;
        vertical_post_x = current_gallows_base_x;         //algoritm pentru pozitionarea in oglinda
        horizontal_beam_start_x = current_gallows_base_x;
        horizontal_beam_end_x = current_gallows_base_x + 100; 
        rope_x = current_gallows_base_x + 100; 
    }

    
    SDL_RenderDrawLine(renderer, base_left_x, gallows_bottom, base_right_x, gallows_bottom);
    // verticala
    SDL_RenderDrawLine(renderer, vertical_post_x, gallows_top, vertical_post_x, gallows_bottom);
    // orizontala
    SDL_RenderDrawLine(renderer, horizontal_beam_start_x, gallows_top, horizontal_beam_end_x, gallows_top);
    // funia
    SDL_RenderDrawLine(renderer, rope_x, gallows_top, rope_x, gallows_top + 25);


    int centerX = rope_x; 
    int centerY = gallows_top + 25 + 25;
    int radius = 25;

    // cap
    if (wrong_guesses >= 1) {
        for (int i = 0; i < 360; i = i + 5) {
            double angle = i * M_PI / 180.0;
            int x1 = centerX + radius * cos(angle);
            int y1 = centerY + radius * sin(angle);
            double next_angle = (i + 5) * M_PI / 180.0;
            int x2 = centerX + radius * cos(next_angle);
            int y2 = centerY + radius * sin(next_angle);
            SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        }
    }
    // corp
    if (wrong_guesses >= 2) {
        SDL_RenderDrawLine(renderer, centerX, gallows_top + 75, centerX, gallows_top + 150);
    }
    // mana stanga
    if (wrong_guesses >= 3) {
        SDL_RenderDrawLine(renderer, centerX, gallows_top + 100, centerX + (mirrored ? 30 : -30), gallows_top + 130);
    }
    //mana dreapta
    if (wrong_guesses >= 4) { 
        SDL_RenderDrawLine(renderer, centerX, gallows_top + 100, centerX + (mirrored ? -30 : 30), gallows_top + 130);
    }
    // picioar stang
    if (wrong_guesses >= 5) { 
        SDL_RenderDrawLine(renderer, centerX, gallows_top + 150, centerX + (mirrored ? 30 : -30), gallows_top + 190);
    }
    //picior drept
    if (wrong_guesses >= 6) {
        SDL_RenderDrawLine(renderer, centerX, gallows_top + 150, centerX + (mirrored ? -30 : 30), gallows_top + 190);
    }
}

bool initialize_game(Game* game) {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        fprintf(stderr, "SDL initialization error: %s\n", SDL_GetError());
        return false;
    }

    if (!(IMG_Init(IMAGE_FLAGS) & IMAGE_FLAGS)) {
        fprintf(stderr, "SDL_image initialization error: %s\n", IMG_GetError());
        return false;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "SDL_ttf initialization error: %s\n", TTF_GetError());
        return false;
    }

    game->window = SDL_CreateWindow(WINDOW_TITLE,            
                                   SDL_WINDOWPOS_CENTERED,   //efectiv fereastra jocului sa fie centrata
                                   SDL_WINDOWPOS_CENTERED,
                                   WIDTH,
                                   HEIGHT,
                                   SDL_WINDOW_SHOWN);
    if (!game->window) {
        fprintf(stderr, "Window creation error: %s\n", SDL_GetError());
        return false;
    }

    game->renderer = SDL_CreateRenderer(game->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!game->renderer) {
        fprintf(stderr, "Renderer creation error: %s\n", SDL_GetError());
        return false;
    }

    game->current_state = MAIN_MENU;
    game->hangman = NULL; // inca suntem in main menu
    game->current_language = LANG_ENGLISH;

    game->flag_rect.w = 60; 
    game->flag_rect.h = 40; 
    game->flag_rect.x = WIDTH - game->flag_rect.w - 10; 
    game->flag_rect.y = 10; 
    return true;
}

bool load_media(Game* game) {
    // initializarea backgroundului
    game->background = IMG_LoadTexture(game->renderer, "images/bg.jpg");
    if (!game->background) {
        fprintf(stderr, "Failed to load background image: %s\n", IMG_GetError());
        return false;
    }

    // font
    game->text_font = TTF_OpenFont("fonts/Freckle_Face/FreckleFace-Regular.ttf", FONT_SIZE);
    if (!game->text_font) {
        fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
        return false;
    }

    game->flag_en_texture = IMG_LoadTexture(game->renderer, "images/flag_en.png");
    if (!game->flag_en_texture) {
        fprintf(stderr, "WARNING: Failed to load English flag: %s\n", IMG_GetError());
    }
    game->flag_ro_texture = IMG_LoadTexture(game->renderer, "images/flag_ro.png");
    if (!game->flag_ro_texture) {
        fprintf(stderr, "WARNING: Failed to load Romanian flag: %s\n", IMG_GetError());
    }


    // culoarea butonului
    SDL_Color button_color = {255, 255, 255, 255}; // Light grey

    // normal Mode Button
    game->buttons[BUTTON_NORMAL_MODE].rect = (SDL_Rect){WIDTH / 2 - 100, 550, 210, 65}; //e de tip SDL_Rect care are parametrii x,y,w,h
    strcpy(game->buttons[BUTTON_NORMAL_MODE].text, "NORMAL MODE"); //se face un dreptunghi si se creeaza textul

    // hard Mode Button
    game->buttons[BUTTON_HARD_MODE].rect = (SDL_Rect){WIDTH / 2 - 100, 610, 210, 65};
    strcpy(game->buttons[BUTTON_HARD_MODE].text, "HARD MODE");
    
    // versus Mode Button
    game->buttons[BUTTON_VERSUS_MODE].rect = (SDL_Rect){WIDTH / 2 - 100, 670, 210, 65};
    strcpy(game->buttons[BUTTON_VERSUS_MODE].text, "VERSUS MODE");

    // language Button
    game->buttons[BUTTON_LANGUAGE].rect = (SDL_Rect){WIDTH / 2 - 100, 730, 210, 65};
    strcpy(game->buttons[BUTTON_LANGUAGE].text, "LANGUAGE");

    //se ia fiecare buton in parte, se face suprafata, se pune font, text, si culoare pe acea suprafata
    for (int i = 0; i < BUTTON_COUNT; i++) {
        SDL_Surface* text_surface = TTF_RenderText_Blended(game->text_font, game->buttons[i].text, button_color);//se ia textu pus si se face
        if (!text_surface) {                                                                                     //suprafata
            fprintf(stderr, "Failed to create text surface for button %s: %s\n", game->buttons[i].text, TTF_GetError());
            return false;
        }
        game->buttons[i].texture = SDL_CreateTextureFromSurface(game->renderer, text_surface);//suprafata se face textura
        if (!game->buttons[i].texture) {
            fprintf(stderr, "Failed to create texture for button %s: %s\n", game->buttons[i].text, SDL_GetError());
            SDL_FreeSurface(text_surface);
            return false;
        }
        SDL_FreeSurface(text_surface); // se elimina suprafata
    }
    
    return true;
}

void cleanup_game(Game* game) {
    normal_mode_cleanup(game);
    hard_mode_cleanup(game);
    versus_mode_cleanup(game);
    
    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (game->buttons[i].texture) {
            SDL_DestroyTexture(game->buttons[i].texture);
            game->buttons[i].texture = NULL; //se elimina fiecare textura creata
        }
    }
    if (game->background) {
        SDL_DestroyTexture(game->background);
        game->background = NULL;
    }
    if (game->text_font) {
        TTF_CloseFont(game->text_font);
        game->text_font = NULL;
    }
    if (game->flag_en_texture) {
        SDL_DestroyTexture(game->flag_en_texture);
        game->flag_en_texture = NULL;
    }
    if (game->flag_ro_texture) {
        SDL_DestroyTexture(game->flag_ro_texture);
        game->flag_ro_texture = NULL;
    }
    if (game->renderer) {
        SDL_DestroyRenderer(game->renderer);
        game->renderer = NULL;
    }
    if (game->window) {
        SDL_DestroyWindow(game->window);
        game->window = NULL;
    }
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}


void handle_events(Game* game) {
    SDL_Event event; // e un union din SDL care are mai multe evenimente si substructuri(evenimente generate de mouse, miscari, tastatura)
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                cleanup_game(game);
                exit(0);
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) { //cand se apasa clickul
                    if (game->current_state == MAIN_MENU) {
                        int mouse_x = event.button.x;   //cand suntem in main menu se iau coordonatele de la apasarea clickului
                        int mouse_y = event.button.y;

                        for (int i = 0; i < BUTTON_COUNT; i++) {
                            if (SDL_PointInRect(&(SDL_Point){mouse_x, mouse_y}, &game->buttons[i].rect)) {
                                                            //se verfica daca clickul este in limitele dreptunghiului butonului
                                normal_mode_cleanup(game); //functia cere 2 pointeri si returneaza un bool,SDL_Point variabila temp ce stocheaza coordonatele
                                hard_mode_cleanup(game);   
                                versus_mode_cleanup(game);
                                switch (i) { //pentru fiecare buton se initializeaza starea
                                    case BUTTON_NORMAL_MODE:
                                        game->current_state = NORMAL_MODE;
                                        normal_mode_init(game); 
                                        break;
                                    case BUTTON_HARD_MODE:
                                        game->current_state = HARD_MODE;
                                        hard_mode_init(game); 
                                        break;
                                    case BUTTON_VERSUS_MODE:
                                        game->current_state = VERSUS_MODE;
                                        versus_mode_init(game);
                                        break;
                                    case BUTTON_LANGUAGE:
                                        game->current_language = (game->current_language == LANG_ENGLISH) ? LANG_ROMANIAN : LANG_ENGLISH;
                                        normal_mode_cleanup(game); 
                                        hard_mode_cleanup(game);   
                                        versus_mode_cleanup(game);

                                }
                                if ((game->current_state == NORMAL_MODE || game->current_state == HARD_MODE || 
                                game->current_state == VERSUS_MODE) && game->hangman == NULL && game->versus_data == NULL) {
                                    fprintf(stderr, "Error at initialing game mode.\n");
                                    game->current_state = MAIN_MENU; // daca da fail sa se initializeze un mod de joc
                                }
                                break;
                            }
                        }
                    } else if (game->current_state == NORMAL_MODE) {
                        normal_mode_handle_event(game, &event);
                    } else if (game->current_state == HARD_MODE) { 
                        hard_mode_handle_event(game, &event);
                    }else if (game->current_state == VERSUS_MODE) { 
                        versus_mode_handle_event(game, &event);
                    }
                }
                break;

            case SDL_MOUSEMOTION: // pentru efectele de hover din main menu
                if (game->current_state == MAIN_MENU) {
                    int mouse_x = event.motion.x;
                    int mouse_y = event.motion.y;
                    for (int i = 0; i < BUTTON_COUNT; i++) { //se verifica daca clickul e in coord lui rect
                        bool is_hovered_now = SDL_PointInRect(&(SDL_Point){mouse_x, mouse_y}, &game->buttons[i].rect);
                        if (game->buttons[i].is_hovered != is_hovered_now) {
                            game->buttons[i].is_hovered = is_hovered_now;
                            // se reface culoarea cand mouse-ul e hovered sau nu
                            SDL_Color current_color = {255, 255, 255, 255}; // alb
                            if (game->buttons[i].is_hovered) {
                                current_color = (SDL_Color){69,192,215,255}; // albastru la hover
                            }
                            if (game->buttons[i].texture) {
                                SDL_DestroyTexture(game->buttons[i].texture); //se distruge textura anterioara si se creeaza una noua
                            }
                            SDL_Surface* text_surface = TTF_RenderText_Blended(game->text_font, game->buttons[i].text, current_color);
                            if (text_surface) {
                                game->buttons[i].texture = SDL_CreateTextureFromSurface(game->renderer, text_surface);
                                SDL_FreeSurface(text_surface);
                            }
                        }
                    }
                }
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    if (game->current_state != MAIN_MENU) {
                        // iese din fiecare game mode si da clean
                        if (game->current_state == NORMAL_MODE) {
                            normal_mode_cleanup(game);
                        } else if (game->current_state == HARD_MODE) {
                            hard_mode_cleanup(game);
                        } else if (game->current_state == VERSUS_MODE){
                            //versus_mode_cleanup(game);
                        }
                        game->current_state = MAIN_MENU; //se updateaza state-ul
                    } else {
                        cleanup_game(game); // daca e in main menu se da clean si iese
                        exit(0);
                    }
                } 
                else if (game->current_state == NORMAL_MODE) {
                    normal_mode_handle_event(game, &event);
                } 
                else if (game->current_state == HARD_MODE) { 
                    hard_mode_handle_event(game, &event);
                }
                else if (game->current_state == VERSUS_MODE) { 
                    versus_mode_handle_event(game, &event);
                }
                break;
        }
    }
}

void render_main_menu(Game* game) {
    SDL_RenderCopy(game->renderer, game->background, NULL, NULL);
    
    for (int i = 0; i < BUTTON_COUNT; i++) {
       
        if (game->buttons[i].texture) {
            SDL_RenderCopy(game->renderer, game->buttons[i].texture, NULL, &game->buttons[i].rect); //afiseaza fiecare buton pe ecran
        } else {
            fprintf(stderr, "Error in main menu for button texture for '%s'\n", game->buttons[i].text);
        }
    }
    SDL_Texture* current_flag_texture = NULL;
    if (game->current_language == LANG_ENGLISH) {
        current_flag_texture = game->flag_en_texture;
    } else { 
        current_flag_texture = game->flag_ro_texture;
    }

    if (current_flag_texture) {
        SDL_RenderCopy(game->renderer, current_flag_texture, NULL, &game->flag_rect);
    } else {
        fprintf(stderr, "WARNING: No flag texture to render for language %d.\n", game->current_language);
    }

}

void render_mode_under_construction(Game* game) {
    SDL_SetRenderDrawColor(game->renderer, 30, 30, 30, 255);
    SDL_RenderClear(game->renderer);

    SDL_Color white = {255, 255, 255, 255};
    render_text(game->renderer, game->text_font, "Mode Under Construction", white,
                (WIDTH - (strlen("Mode Under Construction") * FONT_SIZE / 2)) / 2, (HEIGHT - FONT_SIZE) / 2);
}
void render_keyboard(Game* game) {
    if (game == NULL || game->renderer == NULL || game->hangman == NULL || game->hangman->letter_textures[0] == NULL) {
        fprintf(stderr, "ERROR: render_keyboard: Invalid game data or uninitialized letter textures.\n");
        return;
    }

    SDL_Color border_color = {255, 255, 255, 255};     

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        SDL_Rect key_rect = game->hangman->letter_rects[i];
        SDL_SetRenderDrawColor(game->renderer, border_color.r, border_color.g, border_color.b, border_color.a);
        SDL_RenderDrawRect(game->renderer, &key_rect);

        if (game->hangman->letter_textures[i]) {
            int text_width, text_height;
            SDL_QueryTexture(game->hangman->letter_textures[i], NULL, NULL, &text_width, &text_height);
            SDL_Rect text_dst_rect = {
                key_rect.x + (key_rect.w - text_width) / 2,
                key_rect.y + (key_rect.h - text_height) / 2,
                text_width,
                text_height
            };
            SDL_RenderCopy(game->renderer, game->hangman->letter_textures[i], NULL, &text_dst_rect);
        }
    }
}