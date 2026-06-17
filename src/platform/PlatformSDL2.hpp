#pragma once

#include <iostream>
#include <array>
#include <SDL2/SDL.h>

class PlatformSDL2{

    public:
        PlatformSDL2(const char *title, int x, int y, int window_width, int window_height, int screen_width, int screen_height);
        ~PlatformSDL2();
        void updateScreen(std::array <bool,64*32> Video);
    private:
        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_Texture* texture;
        std::array <uint32_t,64*32> Pixels{};
};