#include <iostream>
#include "Chip8.hpp"

int main()
{
    std::cout << "CHIP-8 Emulator\n";
    Chip8 chip8;
    int n = 0;
    while(true){
        chip8.cycle();
    
    }
    return 0;
}