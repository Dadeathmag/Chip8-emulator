#pragma once

#include <iostream>
#include <array>
#include <SDL2/SDL.h>

class PlatformSDL2{

    public:

        PlatformSDL2(const char *title, int x, int y, int window_width, int window_height, int screen_width, int screen_height);
        ~PlatformSDL2();
        void handleInput(std::array <bool,16>& keys);
        void updateScreen(std::array <bool,64*32> Video);
        void createDelay(int value);
        void makeBeep(bool active);
        
        //acessor-mutator
        bool isRunning();
        uint32_t getTicks();

    private:

        bool running;

        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_Texture* texture;
        SDL_Event event;
        SDL_AudioDeviceID audioDevice{};
        bool beepActive{};
        int beepPhase{};
        std::array <uint32_t,64*32> Pixels{};

        static void audioCallback(void* userdata, Uint8* stream, int len);
        static int mapKey(SDL_Keycode key);

};