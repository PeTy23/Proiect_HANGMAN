#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "interface.h"

int main() {
    Game game = {0};

    if (!initialize_game(&game)) {
        cleanup_game(&game);
        return 1;
    }

    if (!load_media(&game)) {
        cleanup_game(&game);
        return 1;
    }

    bool running = true;
    while (running) {
        handle_events(&game);
        
        SDL_SetRenderDrawColor(game.renderer, 53, 214, 212, 255);
        SDL_RenderClear(game.renderer);

        switch (game.current_state) {
            case MAIN_MENU:
                render_main_menu(&game);
                break;
            case NORMAL_MODE:
                render_normal_mode(&game);
                break;
            case HARD_MODE:
                render_normal_mode(&game);
                break;
            case VERSUS_MODE:
                render_normal_mode(&game);
                break;
            case LANGUAGE:
                render_normal_mode(&game);
                break;

        }

        SDL_RenderPresent(game.renderer);
        SDL_Delay(16);
    }

    cleanup_game(&game);
    return 0;
}