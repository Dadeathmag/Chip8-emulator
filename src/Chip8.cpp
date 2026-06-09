#include <iostream>
#include <fstream>
#include "Chip8.hpp"

#define FONTSET_SIZE 80
#define START_ADDRESS 0x200

//Constructor
Chip8::Chip8(){
    std::cout << "Chip8 instance created" << std::endl;
    reset();    
}


//Resets the Chip8 system to its initial state
void Chip8::reset(){
      
    //Reset registers and memory
    V.fill(0);
    Memory.fill(0);
    Stack.fill(0);
    Video.fill(0);
    Keys.fill(0);

    opcode=0;
    PC=START_ADDRESS;               //Program counter starts at 0x200
    I=0;
    SP=0;
    delay_timer = 0;
    sound_timer = 0;  


    uint8_t font_set[FONTSET_SIZE]={
        0xF0,0x90,0x90,0x90,0xF0,   //0
        0x20,0x60,0x20,0x20,0x70,   //1
        0xF0,0x10,0xF0,0x80,0xF0,   //2
        0xF0,0x10,0xF0,0x10,0xF0,   //3
        0x90,0x90,0xF0,0x10,0x10,   //4
        0xF0,0x80,0xF0,0x10,0xF0,   //5
        0xF0,0x80,0xF0,0x90,0xF0,   //6
        0xF0,0x10,0x20,0x40,0x40,   //7
        0xF0,0x90,0xF0,0x90,0xF0,   //8
        0xF0,0x90,0xF0,0x10,0xF0,   //9
        0xF0,0x90,0xF0,0x90,0x90,   //A
        0xE0,0x90,0xE0,0x90,0xE0,   //B
        0xF0,0x80,0x80,0x80,0xF0,   //C
        0xE0,0x90,0x90,0x90,0xE0,   //D
        0xF0,0x80,0xF0,0x80,0xF0,   //E
        0xF0,0x80,0xF0,0x80,0x80,   //F
    };

    //from 0x050 is prefered by most here from 0x000
    for(int i=0;i<FONTSET_SIZE;i++) Memory[i]=font_set[i];
    std::cout << "Cpu reset" << std::endl;
}

/*
void Chip8::loadROM(const char* filename){
    std::cout << "Loading ROM: " << filename << std::endl;

    FILE* rom = fopen(filename, "rb");                                  
    if(rom == nullptr){
        std::cerr << "Error: Could not open ROM file" << std::endl;
        return;
    }

    // Read ROM data into memory

    fseek(rom, 0, SEEK_END);
    long rom_size = ftell(rom);                                     //determine size of rom
    fseek(rom, 0, SEEK_SET);

    if(rom_size>(4096-START_ADDRESS)){
        std::cerr << "Error: ROM size exceeds available memory" << std::endl;
        fclose(rom);
        return;
    }

    fread(&Memory[START_ADDRESS], sizeof(uint8_t), rom_size, rom);
    fclose(rom);
}
*/

void Chip8::loadROM(const char* filename){
    std::cout << "Loading ROM: " << filename << std::endl;

    std::ifstream rom(filename,std::ios::binary);
    if(!rom){
        std::cerr <<"failed to open ROM\n ";
        return;
    }
    
    rom.seekg(0,std::ios::end);
    std::streamsize romSize = rom.tellg(); //determine size of rom
    rom.seekg(0,std::ios::beg);

    if (romSize > (4096 - START_ADDRESS))
    {
        std::cerr << "ROM too large\n";
        return;
    }

    //Read ROM data into memory
    rom.read(
        reinterpret_cast<char*>(&Memory[START_ADDRESS]),
        romSize
    );

    std::cout << "Loaded " << romSize << " bytes\n";
}

void Chip8::cycle(){
    //Fetch opcode and increment program counter
    opcode =(Memory[PC]<<8u)|Memory[PC+1];
    PC+=2;

    //opcode fields
    uint16_t nnn = opcode & 0x0FFFu;            //address
    uint8_t  nn   = opcode & 0x00FFu;           //8 bit constant
    uint8_t  n    = opcode & 0x000Fu;           //4 bit constant
    uint8_t  x    = (opcode & 0x0F00u) >> 8u;   //register X
    uint8_t  y    = (opcode & 0x00F0u) >> 4u;   //register Y

    switch(opcode & 0xF000u){
        // decode and execute 35 opcodes here
        case 0x0000:
            switch(opcode & 0x00FFu){
                case 0x00E0:
                    //CLS
                    break;
                case 0x00EE:
                    //RET
                    break;
                default:  //0x0nnn
                    //SYS addr ignored
                    break;
            }
            break;
        case 0x1000:
            //JP addr
            break;
        case 0x2000:
            //CALL addr
            break;
        case 0x3000:
            

        default:
            std::cerr<< "Unknown opcode: " << std::hex << opcode << std::endl;
    }

    std::cout << "Cycle Executed" << std::endl;
}
