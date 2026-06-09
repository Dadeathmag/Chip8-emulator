#pragma once
#include <array>
#include <cstdint>

class Chip8{
    public:
        Chip8();
        void reset();
        void loadROM(const char* filename);
        void cycle();
    private:
    
        std::array<uint8_t, 16> V{};                        //16 8bit registers V0 - VE , VF - flag register

        std::array<uint8_t, 4096> Memory{};                 //4k memory

        std::array <uint16_t,16> Stack{};                   //16 16bit stack

        std::array <bool,64*32> Video{};                    //64x32 monochrome display

        std::array <bool, 16> Keys{};                       //16 key input
        
        uint16_t opcode{};                                  //16 bit opcode

        uint16_t PC{};                                      //16 bit program counter

        uint8_t SP{};                                       //8 bit stack pointer

        uint16_t I{};                                       //16 bit index register
        
        //timers
        uint8_t delay_timer{};
        uint8_t sound_timer{};

};