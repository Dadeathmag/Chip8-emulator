#include <iostream>
#include "core/Chip8.hpp"

//debug
void displayInTerminal(std::array <bool,64*32>);
//debug

int main()
{
    //debug
    std::cout << "CHIP-8 Emulator\n enter rom name to run:";
    char rom[20];
    std::cin >>rom;
    int count;
    std::cout << "enter no of cycles:";
    std::cin >>count;
    //debug

    Chip8 chip8;
    chip8.loadROM(rom);

    int n = 0;
    while(true){

        chip8.cycle();
        if(chip8.drawflag){
            chip8.drawflag=false;
            displayInTerminal(chip8.getVideo());
        }
        n++;
    }
    //displayInTerminal(chip8.getVideo());
    return 0;
}

//debug
void displayInTerminal(std::array <bool,64*32> Video){
        for(int y = 0; y < 32; y++)
    {
        for(int x = 0; x < 64; x++)
        {
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