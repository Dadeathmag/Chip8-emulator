
#include "PlatformSDL2.hpp"

PlatformSDL2::PlatformSDL2(const char *title, int x, int y, int window_width, int window_height, int screen_width, int screen_height){
    SDL_Init( SDL_INIT_EVERYTHING );
    window = SDL_CreateWindow(title,x,y,window_width,window_height, SDL_WINDOW_SHOWN );
    renderer = SDL_CreateRenderer(window,0,0);
    texture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, screen_width, screen_height);
    Pixels.fill(0);

    if (!window) {
        std::cout << "Failed to create a window! Error: " << SDL_GetError() << std::endl;
    }
}

PlatformSDL2::~PlatformSDL2(){
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);
    window = NULL;
    renderer=NULL;
    texture=NULL;
    SDL_Quit();
}

void PlatformSDL2::updateScreen(std::array <bool,64*32> Video){
    for(int i = 0; i < 64*32; i++){
                                //black     white
        Pixels[i] = Video[i] ? 0xFF683D12 : 0xFFEC9034;
    }
    SDL_UpdateTexture(texture, NULL, Pixels.data(), 64 * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}