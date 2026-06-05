#pragma once

class Chip8{
    public:
        Chip8();
        void Cycle(int n);
    private:
        
        u_int8_t V[16];
        u_int8_t memory[4096];

};