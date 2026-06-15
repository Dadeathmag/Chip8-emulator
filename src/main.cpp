#include <iostream>
#include "Chip8.hpp"

void displayInTerminal(std::array <bool,64*32>);

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
    while(n < count){
        chip8.cycle();
        n++;
    }
    displayInTerminal(chip8.getVideo());
    return 0;
}

void displayInTerminal(std::array <bool,64*32> Video){
        for(int y = 0; y < 32; y++)
    {
        for(int x = 0; x < 64; x++)
        {
            std::cout << (Video[y * 64 + x] ? '#' : '.');
        }

        std::cout << '\n';
    }
}