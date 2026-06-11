#include <iostream>
#include <fstream>
#include "Chip8.hpp"

#define FONT_START_ADDRESS 0x050        //standardly used by developers
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

    //from 0x050 is prefered by most 
    for(int i=0;i<FONTSET_SIZE;i++) Memory[FONT_START_ADDRESS + i]=font_set[i];
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
                                                //                 Memory 
    opcode = (Memory[PC]<<8u)|Memory[PC+1];      //         XX00<==| XX |<==PC
    PC+=2;                                      //         ^00XX<==| XX | 
                                                //          ----   | XX |<==PC+2    
                                                // opcode = XXXX   | XX |
    //opcode fields
    const uint16_t addr = opcode & 0x0FFFu;            //address
    const uint8_t  byte   = opcode & 0x00FFu;           //byte
    const uint8_t  nibble    = opcode & 0x000Fu;           //nibble
    const uint8_t  x    = (opcode & 0x0F00u) >> 8u;   //register X
    const uint8_t  y    = (opcode & 0x00F0u) >> 4u;   //register Y

    switch(opcode & 0xF000u){
        // decode and execute 35 opcodes here
        case 0x0000:
            switch(opcode & 0x00FFu){
                case 0x00E0:
                    //CLS: clear screen
                    opClearScreen();
                    break;
                case 0x00EE:
                    //RET: return from subroutine
                    opReturn();
                    break;
                default:  //0x0nnn
                    //SYS addr ignored: used by cosmac vip
                    opUnknownOpcode(opcode);
                    break;
            }
            break;
        case 0x1000:
            //JP addr
            opJump(addr);
            break;
        case 0x2000:
            //CALL addr
            opCall(addr);
            break;
        case 0x3000:
            //SE Vx, byte
            opSkip(x,byte);
            break;
        case 0x4000:
            //SNE Vx, byte
            opSkipNot(x,byte);
            break;
        case 0x5000:
            if (nibble == 0){
                // SE Vx, Vy
                opSkipReg(x,y);
            }else{
                opUnknownOpcode(opcode);
            }
            break;
        case 0x6000:
            //LD Vx, byte
            opLoad(x,byte);
            break; 
        case 0x7000:
            //ADD Vx, byte
            opAdd(x,byte);
            break;
        case 0x8000:
            switch(opcode & 0x000Fu){
                case 0x0000:
                    //LD Vx,Vy
                    opLoadReg(x,y);
                    break;
                case 0x0001:
                    //OR Vx,Vy
                    opOR(x,y);
                    break;
                case 0x0002:
                    //AND Vx,Vy
                    opAND(x,y);
                    break;
                case 0x0003:
                    //XOR Vx,Vy
                    opXOR(x,y);
                    break;
                case 0x0004:
                    //ADD Vx,Vy
                    opAddCarry(x,y);
                    break;
                case 0x0005:
                    //SUB Vx,Vy
                    opSub(x,y);
                    break;
                case 0x0006:
                    //SHR Vx,Vy
                    opSHR(x);
                    break;
                case 0x0007:
                    //SUBN Vx,Vy
                    opSubN(x,y);
                    break;
                case 0x000E:
                    //SHL Vx,Vy
                    opSHL(x);
                    break;
                default:
                    opUnknownOpcode(opcode);
            }
            break;
        case 0x9000:
            if (nibble == 0){
                // SNE Vx, Vy
                opSkipNotReg(x,y);
            }else{
                opUnknownOpcode(opcode);
            }
            break;
        case 0xA000:
            //LD I,addr
            opSetI(addr);
            break;
        case 0xB000:
            //JP V0,addr: Jump to addr+V0
            opJumpOffset(addr);
            break;
        case 0xC000:
            //RND Vx,byte
            opSetRandom(x,byte);
            break;
        case 0xD000:
            //DRW Vx,Vy,nibble
            break;
        case 0xE000:
            switch(opcode & 0x00FFu){
                case 0x009E:
                    //SKP Vx
                    break;
                case 0x00A1:
                    //SKNP Vx
                    break;
                default:
                    opUnknownOpcode(opcode);
            }
            break;
        case 0xF000:
            switch(opcode & 0x00FF){
                case 0x0007:
                    //LD Vx, DT
                    break;
                case 0x000A:
                    //LD Vx, K
                    break;
                case 0x0015:
                    //LD DT, Vx
                    break;
                case 0x0018:
                    //LD ST, Vx
                    break;
                case 0x001E:
                    //ADD I, Vx
                    break;
                case 0x0029:
                    //LD F, Vx
                    break;
                case 0x0033:
                    //LD B, Vx
                    break;
                case 0x0055:
                    //LD [I], Vx
                    break;
                case 0x0065:
                    //LD Vx, [I]
                    break;
                default:
                    opUnknownOpcode(opcode);
            }
            break;

        default:
            opUnknownOpcode(opcode);
    }

    std::cout << "Cycle Executed" << std::endl;
}

void Chip8::opUnknownOpcode(uint16_t opcode){
    std::cerr << "Unknown opcode: 0x"
              << std::hex << opcode
              << std::dec
              << '\n';
}

void Chip8::opClearScreen(){
    Video.fill(0);
}

void Chip8::opReturn(){
    SP--;
    PC = Stack[SP];
}

void Chip8::opJump(uint16_t addr){
    PC=addr;
}

void Chip8::opCall(uint16_t addr){
    Stack[SP]=PC;
    SP++;
    PC=addr;
}

void Chip8::opSkip(uint8_t x,uint8_t byte){
    if(V[x]==byte) PC+=2;
}

void Chip8::opSkipNot(uint8_t x,uint8_t byte){
    if(V[x]!=byte) PC+=2;
}

void Chip8::opSkipReg(uint8_t x,uint8_t y){
    if(V[x]==V[y]) PC+=2;
}

void Chip8::opLoad(uint8_t x,uint8_t byte){
    V[x]=byte;
}

void Chip8::opAdd(uint8_t x,uint8_t byte){
    V[x]+=byte;
}

void Chip8::opLoadReg(uint8_t x,uint8_t y){
    V[x]=V[y];
}

void Chip8::opOR(uint8_t x,uint8_t y){
    V[x]|=V[y];
}

void Chip8::opAND(uint8_t x,uint8_t y){
    V[x]&=V[y];
}

void Chip8::opXOR(uint8_t x,uint8_t y){
    V[x]^=V[y];
}

void Chip8::opAddCarry(uint8_t x,uint8_t y){
    uint16_t sum = V[x]+V[y];       //sum max would need 9 bits:255+255=510 
    V[0xF]=(sum>0xFF);              //check overflow set flag
    //V[x]=sum & 0x00FF;            //lowest 8 bits of sum 
    V[x]=static_cast<uint8_t>(sum); //type cast to lowest 8bits
}

void Chip8::opSub(uint8_t x,uint8_t y){
    V[0xF]=(V[x]>V[y]);
    V[x]-=V[y];
}

void Chip8::opSHR(uint8_t x /*,uint8_t y*/){
    //cosmac vip operates on y and store it on x
    V[0xF]=V[x] & 0x01;         //sets flag to least significant bit
    V[x]>>=1;                       
}

void Chip8::opSubN(uint8_t x,uint8_t y){
    V[0xF]=(V[y]>V[x]);
    V[x]=V[y]-V[x];
}

void Chip8::opSHL(uint8_t x /*,uint8_t y*/){
    V[0xF]=V[x] >> 7;         //sets flag to most significant bit
    V[x]<<=1;
}

void Chip8::opSkipNotReg(uint8_t x,uint8_t y){
    if(V[x]!=V[y])PC+=2;
}

void Chip8::opSetI(uint16_t addr){
    I=addr;
}

void Chip8::opJumpOffset(uint16_t addr){
    PC=addr + V[0x0];
}

void Chip8::opSetRandom(uint8_t x,uint8_t byte){
    V[x]= static_cast<uint8_t>(dist(rng)) & byte;
}

void Chip8::opDraw(uint8_t x,uint8_t y,uint8_t nibble){

    V[0xF]=0X00;                              //initialise flag to check collision

    //read through sprite
    for(int i=0;i<nibble;i++){
        uint8_t byte = Memory[I+i];
        for(int j=0;j<8;j++){
            bool bit=(byte>>(7-j)) & 0x01;    //sprite pixel at (j,i)

            //skips if sprite is zero
            if(bit){
                
                //(x+j,y+i)=>on Screen coordinates
                uint8_t screenX=(V[x]+j)%64;                            
                uint8_t screenY=64*((V[y]+i)%32);                       
                uint16_t index=screenX+screenY ;                        
                                    
                //collision detection and drawing
                if(Video[index] && bit)V[0xF]=0x01;
                Video[index]= Video[index] ^ bit;  
            }
        }
    }
}