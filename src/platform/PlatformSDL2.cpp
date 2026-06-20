
#include "PlatformSDL2.hpp"

namespace {
    constexpr int BEEP_FREQUENCY = 440;
    constexpr int BEEP_AMPLITUDE = 2000;
    constexpr uint32_t SCREEN_ON=0xFF683D12;
    constexpr uint32_t SCREEN_OFF=0xFFEC9034;
}

int PlatformSDL2::mapKey(SDL_Keycode key){
    switch(key){
        case SDLK_1: return 0x1;
        case SDLK_2: return 0x2;
        case SDLK_3: return 0x3;
        case SDLK_4: return 0xC;

        case SDLK_q: return 0x4;
        case SDLK_w: return 0x5;
        case SDLK_e: return 0x6;
        case SDLK_r: return 0xD;

        case SDLK_a: return 0x7;
        case SDLK_s: return 0x8;
        case SDLK_d: return 0x9;
        case SDLK_f: return 0xE;

        case SDLK_z: return 0xA;
        case SDLK_x: return 0x0;
        case SDLK_c: return 0xB;
        case SDLK_v: return 0xF;

        default: return -1;
    }
}

void PlatformSDL2::audioCallback(void* userdata, Uint8* stream, int len){
    auto* platform = static_cast<PlatformSDL2*>(userdata);
    auto* buffer = reinterpret_cast<Sint16*>(stream);
    const int samples = len / static_cast<int>(sizeof(Sint16));
    const int halfPeriod = 44100 / BEEP_FREQUENCY / 2;

    for(int i = 0; i < samples; ++i){
        if(platform->beepActive){
            buffer[i] = (platform->beepPhase < halfPeriod) ? BEEP_AMPLITUDE : -BEEP_AMPLITUDE;
            platform->beepPhase = (platform->beepPhase + 1) % (halfPeriod * 2);
        }else{
            buffer[i] = 0;
        }
    }
}

PlatformSDL2::PlatformSDL2(const char *title, int x, int y, int window_width, int window_height, int screen_width, int screen_height){
    SDL_Init(SDL_INIT_EVERYTHING);
    running = true;
    window = SDL_CreateWindow(title, x, y, window_width, window_height, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, 0, 0);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, screen_width, screen_height);
    Pixels.fill(0);

    if(!window){
        std::cout << "Failed to create a window! Error: " << SDL_GetError() << std::endl;
    }

    SDL_AudioSpec want{};
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 256;
    want.callback = audioCallback;
    want.userdata = this;

    SDL_AudioSpec have{};
    audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if(audioDevice == 0){
        std::cout << "Failed to open audio device! Error: " << SDL_GetError() << std::endl;
    }else{
        SDL_PauseAudioDevice(audioDevice, 0);
    }
}

PlatformSDL2::~PlatformSDL2(){
    if(audioDevice != 0){
        SDL_CloseAudioDevice(audioDevice);
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    window = nullptr;
    renderer = nullptr;
    texture = nullptr;
    SDL_Quit();
}

void PlatformSDL2::handleInput(std::array <bool,16>& keys){
    while(SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_QUIT:
                running = false;
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                const int chipKey = mapKey(event.key.keysym.sym);
                if(chipKey >= 0){
                    keys[chipKey] = (event.type == SDL_KEYDOWN);
                }
                break;
            }

            case SDL_WINDOWEVENT:
                if(event.window.event == SDL_WINDOWEVENT_FOCUS_LOST){
                    keys.fill(false);
                }
                break;
        }
    }
}

void PlatformSDL2::updateScreen(std::array <bool,64*32> Video){
    for(int i = 0; i < 64 * 32; i++){
        Pixels[i] = Video[i] ? SCREEN_ON : SCREEN_OFF;
    }
    SDL_UpdateTexture(texture, nullptr, Pixels.data(), 64 * static_cast<int>(sizeof(uint32_t)));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void PlatformSDL2::createDelay(int value){
    SDL_Delay(value);
}

void PlatformSDL2::makeBeep(bool active){
    beepActive = active;
    if(!active){
        beepPhase = 0;
    }
}

bool PlatformSDL2::isRunning(){ return running; }
uint32_t PlatformSDL2::getTicks(){ return SDL_GetTicks(); }
