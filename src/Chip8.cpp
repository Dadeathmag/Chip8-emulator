#include <iostream>
#include "Chip8.hpp"

Chip8::Chip8(){
    std::cout << "Chip8 Initialized" << std::endl;    
}

void Chip8::Cycle(int n){
    std::cout << "Cycle Executed: " << n << std::endl;
}
