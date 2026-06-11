#pragma once
#include <array>
#include <cstdint>
#include <random>

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
        
        //Timers
        uint8_t delay_timer{};
        uint8_t sound_timer{};

        //Random
        std::mt19937 rng{std::random_device{}()};
        std::uniform_int_distribution<uint16_t> dist{0,255};

        //Operations
        void opUnknownOpcode(uint16_t opcode);
        void opClearScreen();
        void opReturn();
        void opJump(uint16_t addr);
        void opCall(uint16_t addr);
        void opSkip(uint8_t x,uint8_t byte);
        void opSkipNot(uint8_t x,uint8_t byte);
        void opSkipReg(uint8_t x,uint8_t y);
        void opLoad(uint8_t x,uint8_t byte);
        void opAdd(uint8_t x,uint8_t byte);
        void opLoadReg(uint8_t x,uint8_t y);
        void opOR(uint8_t x,uint8_t y);
        void opAND(uint8_t x,uint8_t y);
        void opXOR(uint8_t x,uint8_t y);
        void opAddCarry(uint8_t x,uint8_t y);
        void opSub(uint8_t x,uint8_t y);
        void opSHR(uint8_t x /*,uint8_t y*/);
        void opSubN(uint8_t x,uint8_t y);
        void opSHL(uint8_t x /*,uint8_t y*/);
        void opSkipNotReg(uint8_t x,uint8_t byte);
        void opSetI(uint16_t addr);
        void opJumpOffset(uint16_t addr);
        void opSetRandom(uint8_t x,uint8_t byte);
        void opDraw(uint8_t x,uint8_t y,uint8_t nibble);
};