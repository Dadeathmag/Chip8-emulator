#include <iostream>
#include "Chip8.hpp"

int main()
{
    std::cout << "CHIP-8 Emulator\n";
    Chip8 chip8;
    chip8.loadROM("test.ch8");
    int n = 0;
    while(n < 20){
        chip8.cycle();
        n++;
    }
    std::array <bool,64*32> Video = chip8.getVideo();
    for(int y = 0; y < 32; y++)
    {
        for(int x = 0; x < 64; x++)
        {
            std::cout << (Video[y * 64 + x] ? '#' : '.');
        }

        std::cout << '\n';
    }
    return 0;
}