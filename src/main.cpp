#include <iostream>
#include "Chip8.hpp"

int main()
{
    std::cout << "CHIP-8 Emulator\n";
    Chip8 chip8;
    int n = 0;
    while(n < 10){
        chip8.cycle();
        n++;
    }
    return 0;
}