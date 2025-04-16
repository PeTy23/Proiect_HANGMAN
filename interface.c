#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <time.h>
#include <stdlib.h>
#include "interface.h"

#define WINDOW_TITLE "HANGMAN"
#define WIDTH 800
#define HEIGHT 600
#define IMAGE_FLAGS IMG_INIT_PNG
#define FONT_SIZE 40


bool initialize_game(Game* game) {
    //initializez tot ce tine de SDL normal
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        fprintf(stderr, "SDL initialization error: %s\n", SDL_GetError());
        return false;
    }

    // initializez SDL_image
    if (!(IMG_Init(IMAGE_FLAGS) & IMAGE_FLAGS)) {
        fprintf(stderr, "SDL_image initialization error: %s\n", IMG_GetError());
        return false;
    }

    // initializez sdl_tff (font,text)
    if (TTF_Init() < 0) {
        fprintf(stderr, "SDL_ttf initialization error: %s\n", TTF_GetError());
        return false;
    }

    //creez window
    game->window = SDL_CreateWindow(WINDOW_TITLE,
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   WIDTH,
                                   HEIGHT,
                                   SDL_WINDOW_SHOWN);
    if (!game->window) {
        fprintf(stderr, "Window creation error: %s\n", SDL_GetError());
        return false;
    }

    // renderer
    game->renderer = SDL_CreateRenderer(game->window, -1, SDL_RENDERER_ACCELERATED);
    if (!game->renderer) {
        fprintf(stderr, "Renderer creation error: %s\n", SDL_GetError());
        return false;
    }

    // celelalte elemente
    game->background = NULL;
    game->text_font = NULL;
    game->text_color = (SDL_Color){255, 255, 255, 255};
    game->current_state = MAIN_MENU;
    game->hangman_texture = NULL;
    for (int i = 0; i < BUTTON_COUNT; i++) {
        game->buttons[i].texture = NULL; //fiecare textura pentru fiecare buton
    }

    return true;
}

void cleanup_game(Game* game) {
    if (game) {
        // dau free la texturi
        if (game->background) {
            SDL_DestroyTexture(game->background);
            game->background = NULL;
        }
        
        if (game->hangman_texture) {
            SDL_DestroyTexture(game->hangman_texture);
            game->hangman_texture = NULL;
        }
        
        // free la butoane
        for (int i = 0; i < BUTTON_COUNT; i++) {
        destroy_button(&game->buttons[i]);
        }

        
        // la font
        if (game->text_font) {
            TTF_CloseFont(game->text_font);
            game->text_font = NULL;
        }
        
        // Free renderer si window
        if (game->renderer) {
            SDL_DestroyRenderer(game->renderer);
            game->renderer = NULL;
        }
        
        if (game->window) {
            SDL_DestroyWindow(game->window);
            game->window = NULL;
        }
        
        // si quit la text,imagini si sdl
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
    }
}

Button create_button(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y) {
    Button btn = {0};
    btn.rect.x = x;
    btn.rect.y = y;
    btn.rect.w = 300;
    btn.rect.h = 80;
    btn.is_hovered = false;
    strncpy(btn.text, text, sizeof(btn.text) - 1);
    btn.text[sizeof(btn.text) - 1] = '\0';

    // se creeaza o suprafata, butonul initial alb
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, (SDL_Color){255, 255, 255, 255});
    if (surface) {
        btn.texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    return btn;
}

//se da clear la buton
void destroy_button(Button* button) {
    if (button && button->texture) {
        SDL_DestroyTexture(button->texture);
        button->texture = NULL;
    }
}

void render_button(SDL_Renderer* renderer, Button* button) {
    if (!button || !button->texture){
        fprintf(stderr, "error at button render: %s\n", SDL_GetError());
        return;
    }

    int textW, textH;
    SDL_QueryTexture(button->texture, NULL, NULL, &textW, &textH); //se obtine W si H al textului
    SDL_Rect text_rect = {
        button->rect.x + (button->rect.w - textW)/2,
        button->rect.y + (button->rect.h - textH)/2,
        textW,
        textH
    }; // se calculeaza ca dimensiunile textului sa fie centrate cu butonul

    // schimb culoare textului depinzand de mouse
    if (button->is_hovered) {
        SDL_SetTextureColorMod(button->texture, 255, 255, 0); // galben cand se tine mouse-ul
    } else {
        SDL_SetTextureColorMod(button->texture, 255, 255, 255); // alb cand nu
    }
    
    SDL_RenderCopy(renderer, button->texture, NULL, &text_rect); //afiseaza textul, il randeaza pe ecran
}

bool load_buttons(Game* game) {
    struct {
        ButtonType type;
        const char* text;
        int x, y;
    } button_defs[BUTTON_COUNT] = {
        {BUTTON_NORMAL_MODE, "NORMAL MODE", 250, 350},
        {BUTTON_HARD_MODE,   "HARD MODE",   250, 400},  // AICI SA FAC SA FIE MEREU CENTRATE INDIFERENT DE REZOLUTIE
        {BUTTON_VERSUS_MODE, "VERSUS MODE", 250, 450},
        {BUTTON_LANGUAGE,    "LANGUAGE",    250, 500}
    };

    for (int i = 0; i < BUTTON_COUNT; i++) {
        game->buttons[button_defs[i].type] = create_button(
            game->renderer,
            game->text_font,
            button_defs[i].text,
            button_defs[i].x,
            button_defs[i].y
        ); //se ia fiecare buton in parte si se creeaza, fiecare cu parametrii lui
        if (!game->buttons[button_defs[i].type].texture) {
            fprintf(stderr, "error at load button: %s\n", button_defs[i].text);
            return false;
        }
    }
    return true;
}


bool load_media(Game* game) {
    // se atribuie butoanele, poza de fundal, fontul
    game->background = IMG_LoadTexture(game->renderer, "images/bg.png");
    if (!game->background) {
        fprintf(stderr, "error at loading background: %s\n", IMG_GetError());
        return false;
    }

    // Load font
    game->text_font = TTF_OpenFont("fonts/Freckle_Face/FreckleFace-Regular.ttf", FONT_SIZE);
    if (!game->text_font) {
        fprintf(stderr, "error at loading font: %s\n", TTF_GetError());
        return false;
    }

    if (!load_buttons(game)) {
        return false;
    }

    return true;
}

void render_main_menu(Game* game) {
    // seteaza fundalul 
    SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
    SDL_RenderClear(game->renderer);

    // background-ul
    if (game->background) {
        SDL_RenderCopy(game->renderer, game->background, NULL, NULL);
    }

    // se pune titlul
    if (game->text_font) {
        SDL_Surface* title_surface = TTF_RenderText_Blended(game->text_font, "HANGMAN", (SDL_Color){255, 0, 0, 255});
        if (title_surface) {
            SDL_Texture* title_texture = SDL_CreateTextureFromSurface(game->renderer, title_surface); //se converteste suprafata in textura
            if (title_texture) {                                                                    //pt a putea fi desenata
                SDL_Rect title_rect = {
                    (WIDTH - title_surface->w)/2, //seteaza x-ul incat sa fie centrat pe axa x
                    100,        //y
                    title_surface->w,    //dimensiunile sunt extrase de functia de renderText
                    title_surface->h
                };
                SDL_RenderCopy(game->renderer, title_texture, NULL, &title_rect);
                SDL_DestroyTexture(title_texture);
            }
            SDL_FreeSurface(title_surface);
        }
    }

    // deseneaza fiecare buton
    for (int i = 0; i < BUTTON_COUNT; i++) {
        render_button(game->renderer, &game->buttons[i]);
    }

}

void render_normal_mode(Game* game) {

    SDL_SetRenderDrawColor(game->renderer, 30, 30, 30, 255);
    SDL_RenderClear(game->renderer);

    //la fel ca mai sus dar ar trebui sa fac o functie care randeaza fiecare mod de joc
    if (game->text_font) {
        SDL_Surface* surface = TTF_RenderText_Blended(game->text_font, "NORMAL MODE", (SDL_Color){255, 255, 0, 255});
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(game->renderer, surface);
            if (texture) {
                SDL_Rect rect = {
                    (WIDTH - surface->w)/2,
                    50,
                    surface->w,
                    surface->h
                };
                SDL_RenderCopy(game->renderer, texture, NULL, &rect);
                SDL_DestroyTexture(texture);
            }
            SDL_FreeSurface(surface);
        }
    }

    SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(game->renderer, 400, 150, 400, 400); // Gallows pole
    SDL_RenderDrawLine(game->renderer, 300, 150, 500, 150); // Gallows top
    SDL_RenderDrawLine(game->renderer, 400, 150, 400, 200); // Rope
}

void handle_events(Game* game) {
    if (!game) return;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                cleanup_game(game);
                exit(0);
                break;

            case SDL_MOUSEMOTION:
                    if (game->current_state == MAIN_MENU) {
                        for (int i = 0; i < BUTTON_COUNT; i++) {
                            // cand sunt in main menu si cand mouse-ul atinge un text
                            int textW, textH;
                            SDL_QueryTexture(game->buttons[i].texture, NULL, NULL, &textW, &textH);//extrage dimensiunile textului
                            SDL_Rect text_rect = {
                                game->buttons[i].rect.x + (game->buttons[i].rect.w - textW)/2,
                                game->buttons[i].rect.y + (game->buttons[i].rect.h - textH)/2,
                                textW,
                                textH
                            }; //creeaza pt fiecare buton un chenar echivalent cu dimensiunile butonului
                            
                            game->buttons[i].is_hovered =                     //seteaza is_hovered pe true daca clickul e in cadrul butonului
                                (event.motion.x >= text_rect.x &&             
                                event.motion.x <= text_rect.x + text_rect.w &&
                                event.motion.y >= text_rect.y && 
                                event.motion.y <= text_rect.y + text_rect.h); 
                        }
                    }
                    break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT &&   //daca se apasa click stanga si sunt in main menu
                    game->current_state == MAIN_MENU) {
                    for (int i = 0; i < BUTTON_COUNT; i++) {
                        if (event.button.x >= game->buttons[i].rect.x && 
                            event.button.x <= game->buttons[i].rect.x + game->buttons[i].rect.w &&
                            event.button.y >= game->buttons[i].rect.y && 
                            event.button.y <= game->buttons[i].rect.y + game->buttons[i].rect.h) {
                            //se verifica daca coordonatele clickului sunt in interiorul butonului
                            // daca sunt, atunci se verifica ce buton a fost apasat
                            switch (i) {
                                case BUTTON_NORMAL_MODE:
                                    game->current_state = NORMAL_MODE;
                                    break;
                                case BUTTON_HARD_MODE:
                                    game->current_state = NORMAL_MODE; //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                                    break;
                                case BUTTON_VERSUS_MODE:
                                    game->current_state =NORMAL_MODE; //SA MODIFIC AICI PENTRU FIECARE MENIU
                                    break;
                                case BUTTON_LANGUAGE:
                                    game->current_state = NORMAL_MODE;
                                    break;
                            }
                        }
                    }
                }
                break;
   
            case SDL_KEYDOWN:
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    if (game->current_state == NORMAL_MODE) {
                        game->current_state = MAIN_MENU;      //SI AICI SA FAC PENTRU FIECARE MENIU IN PARTE
                    } else {
                        cleanup_game(game);
                        exit(0);
                    }
                }
                break;
        }
    }
}