#include <iostream>
#include "core/Chip8.hpp"
#include "platform/PlatformSDL2.hpp"

#define TITLE "CHIP-8-Emulator"
#define WINDOWWIDTH 512
#define WINDOWHEIGHT 256
#define SCREENWIDTH 64
#define SCREENHEIGHT 32



//debug
void displayInTerminal(std::array <bool,64*32>);
//debug

int main()
{
    PlatformSDL2 sdl(TITLE,700,300,WINDOWWIDTH,WINDOWHEIGHT,SCREENWIDTH,SCREENHEIGHT);

    //debug
    std::cout << "CHIP-8 Emulator\n enter rom name to run:";
    char rom[20];
    std::cin >>rom;
    //debug

    Chip8 chip8;
    chip8.loadROM(rom);

    uint32_t cpuaccumulator = 0;
    uint32_t timeraccumulator = 0;
    uint32_t renderaccumulator = 0;
    bool needsRedraw = false;
    uint32_t lasttime = sdl.getTicks();

    while(sdl.isRunning()){
        uint32_t currenttime = sdl.getTicks();
        uint32_t deltatime = currenttime - lasttime;
        lasttime = currenttime;

        // Fixed-point timing avoids integer truncation (1000/700 -> 1 ms bug)
        cpuaccumulator += deltatime * static_cast<uint32_t>(chip8.cpufrequency);
        timeraccumulator += deltatime * 60;

        sdl.handleInput(chip8.getKeys());

        while(cpuaccumulator >= 1000){
            chip8.cycle();
            cpuaccumulator -= 1000;
            if(chip8.drawflag){
                needsRedraw = true;
                chip8.drawflag = false;
            }
        }

        while(timeraccumulator >= 1000){
            chip8.updateTimers();
            timeraccumulator -= 1000;
        }

        sdl.makeBeep(chip8.soundflag);

        // 1dcell uses DRW for collision probes; only present at ~60 Hz
        renderaccumulator += deltatime;
        if(needsRedraw && renderaccumulator >= 16){
            sdl.updateScreen(chip8.getVideo());
            needsRedraw = false;
            renderaccumulator -= 16;
        }
    }
    //displayInTerminal(chip8.getVideo());
    return 0;
}


//debug
void displayInTerminal(std::array <bool,64*32> Video){
    for(int y = 0; y < 32; y++){
        for(int x = 0; x < 64; x++){
            std::cout << (Video[y * 64 + x] ? '#' : '.');
        }

        std::cout << '\n';
    }
    //std::cout << "\x1B[2J\x1B[H";
    #ifdef _WIN32
        std::system("cls");
    #else
        std::system("clear");
    #endif
}
//debug