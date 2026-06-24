// CHIP-8 CPU implementation: memory map, opcode dispatch, and instruction handlers.
//
// Memory layout:
//   0x000–0x04F  reserved (font set loaded at 0x050)
//   0x050–0x09F  built-in hex font (16 chars × 5 bytes)
//   0x200–0xFFF  program ROM and runtime data

#include <iostream>
#include <fstream>
#include "Chip8.hpp"

#define FONT_START_ADDRESS 0x050  // conventional location for the hex font
#define FONTSET_SIZE 80           // 16 sprites × 5 bytes each
#define START_ADDRESS 0x200       // programs load here
#define DEFAULT_CPU_FREQUENCY 700  

//Quirks
#define VF_RESET true
#define MEMORY_INC true
#define DISP_WAIT true // idu about this
#define CLIPPING true
#define SHIFTING false
#define JUMP_WITH_OFFSET false

//debug
int cycleCount=0;
void debugLog(int cycle,uint16_t opcode,int instruction,uint16_t PC,uint16_t I){
    std::string code;
    switch(instruction){
        case 1:  code = "CLS"; break;
        case 2:  code = "RET"; break;
        case 3:  code = "JP addr"; break;
        case 4:  code = "CALL addr"; break;
        case 5:  code = "SE Vx, byte"; break;
        case 6:  code = "SNE Vx, byte"; break;
        case 7:  code = "SE Vx, Vy"; break;
        case 8:  code = "LD Vx, byte"; break;
        case 9:  code = "ADD Vx, byte"; break;

        case 10: code = "LD Vx, Vy"; break;
        case 11: code = "OR Vx, Vy"; break;
        case 12: code = "AND Vx, Vy"; break;
        case 13: code = "XOR Vx, Vy"; break;
        case 14: code = "ADD Vx, Vy"; break;
        case 15: code = "SUB Vx, Vy"; break;
        case 16: code = "SHR Vx"; break;
        case 17: code = "SUBN Vx, Vy"; break;
        case 18: code = "SHL Vx"; break;

        case 19: code = "SNE Vx, Vy"; break;
        case 20: code = "LD I, addr"; break;
        case 21: code = "JP V0, addr"; break;
        case 22: code = "RND Vx, byte"; break;
        case 23: code = "DRW Vx, Vy, nibble"; break;

        case 24: code = "SKP Vx"; break;
        case 25: code = "SKNP Vx"; break;

        case 26: code = "LD Vx, DT"; break;
        case 27: code = "LD Vx, K"; break;
        case 28: code = "LD DT, Vx"; break;
        case 29: code = "LD ST, Vx"; break;
        case 30: code = "ADD I, Vx"; break;
        case 31: code = "LD F, Vx"; break;
        case 32: code = "LD B, Vx"; break;
        case 33: code = "LD [I], Vx"; break;
        case 34: code = "LD Vx, [I]"; break;

        default:
            code = "UNKNOWN";
            break;
    }
    std::cout<<"\ncount:"<<cycle<<" opcode:"<<std::hex << opcode<<"\ninstruction:"<<code<<"\nPC:"<<PC<<" I:"<<I<<std::dec<<std::endl;
    cycleCount++;
}
//debug

//Constructor
Chip8::Chip8(){
    std::cout << "Chip8 instance created" << std::endl;
    cpufrequency=DEFAULT_CPU_FREQUENCY;    
}

//Resets the Chip8 system to its initial state
void Chip8::reset(){
      
    //Reset registers and memory
    V.fill(0);
    Memory.fill(0);
    Stack.fill(0);
    Video.fill(0);
    Keys.fill(0);
    PrevKeys.fill(0);

    opcode=0;
    PC=START_ADDRESS;               //Program counter starts at 0x200
    I=0;
    SP=0;
    delay_timer = 0;
    sound_timer = 0;  
    drawflag=0;
    soundflag=0;

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
    //reset cpu first 
    reset();

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

    //debug
    int instruction=0;
    //debug

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
                    instruction=1;
                    break;
                case 0x00EE:
                    //RET: return from subroutine
                    opReturn();
                    instruction=2;
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
            instruction=3;
            break;
        case 0x2000:
            //CALL addr
            opCall(addr);
            instruction=4;
            break;
        case 0x3000:
            //SE Vx, byte
            opSkip(x,byte);
            instruction=5;
            break;
        case 0x4000:
            //SNE Vx, byte
            opSkipNot(x,byte);
            instruction=6;
            break;
        case 0x5000:
            if (nibble == 0){
                // SE Vx, Vy
                opSkipReg(x,y);
                instruction=7;
            }else{
                opUnknownOpcode(opcode);
            }
            break;
        case 0x6000:
            //LD Vx, byte
            opLoad(x,byte);
            instruction=8;
            break; 
        case 0x7000:
            //ADD Vx, byte
            opAdd(x,byte);
            instruction=9;
            break;
        case 0x8000:
            switch(opcode & 0x000Fu){
                case 0x0000:
                    //LD Vx,Vy
                    opLoadReg(x,y);
                    instruction=10;
                    break;
                case 0x0001:
                    //OR Vx,Vy
                    opOR(x,y);
                    instruction=11;
                    break;
                case 0x0002:
                    //AND Vx,Vy
                    opAND(x,y);
                    instruction=12;
                    break;
                case 0x0003:
                    //XOR Vx,Vy
                    opXOR(x,y);
                    instruction=13;
                    break;
                case 0x0004:
                    //ADD Vx,Vy
                    opAddCarry(x,y);
                    instruction=14;
                    break;
                case 0x0005:
                    //SUB Vx,Vy
                    opSub(x,y);
                    instruction=15;
                    break;
                case 0x0006:
                    //SHR Vx,Vy
                    opSHR(x,y);
                    instruction=16;
                    break;
                case 0x0007:
                    //SUBN Vx,Vy
                    opSubN(x,y);
                    instruction=17;
                    break;
                case 0x000E:
                    //SHL Vx,Vy
                    opSHL(x,y);
                    instruction=18;
                    break;
                default:
                    opUnknownOpcode(opcode);
            }
            break;
        case 0x9000:
            if (nibble == 0){
                // SNE Vx, Vy
                opSkipNotReg(x,y);
                instruction=19;
            }else{
                opUnknownOpcode(opcode);
            }
            break;
        case 0xA000:
            //LD I,addr
            opSetI(addr);
            instruction=20;
            break;
        case 0xB000:
            //JP V0,addr: Jump to addr+V0
            opJumpOffset(addr,x);
            instruction=21;
            break;
        case 0xC000:
            //RND Vx,byte
            opSetRandom(x,byte);
            instruction=22;
            break;
        case 0xD000:
            //DRW Vx,Vy,nibble
            opDraw(x,y,nibble);
            instruction=23;
            break;
        case 0xE000:
            switch(opcode & 0x00FFu){
                case 0x009E:
                    //SKP Vx
                    opSkipKey(x);
                    instruction=24;
                    break;
                case 0x00A1:
                    //SKNP Vx
                    opSkipKeyNot(x);
                    instruction=25;
                    break;
                default:
                    opUnknownOpcode(opcode);
            }
            break;
        case 0xF000:
            switch(opcode & 0x00FF){
                case 0x0007:
                    //LD Vx, DT
                    opGetDT(x);
                    instruction=26;
                    break;
                case 0x000A:
                    //LD Vx, K
                    opLoadKeyPress(x);
                    instruction=27;
                    break;
                case 0x0015:
                    //LD DT, Vx
                    opSetDT(x);
                    instruction=28;
                    break;
                case 0x0018:
                    //LD ST, Vx
                    opSetST(x);
                    instruction=29;
                    break;
                case 0x001E:
                    //ADD I, Vx
                    opAddI(x);
                    instruction=30;
                    break;
                case 0x0029:
                    //LD F, Vx
                    opPointSprite(x);
                    instruction=31;
                    break;
                case 0x0033:
                    //LD B, Vx
                    opBCD(x);
                    instruction=32;
                    break;
                case 0x0055:
                    //LD [I], Vx
                    opStoreMem(x);
                    instruction=33;
                    break;
                case 0x0065:
                    //LD Vx, [I]
                    opLoadMem(x);
                    instruction=34;
                    break;
                default:
                    opUnknownOpcode(opcode);
            }
            break;

        default:
            opUnknownOpcode(opcode);
    }

    //debugLog(cycleCount,opcode,instruction,PC,I);
}

//acessor-mutator

std::array <bool,64*32> Chip8::getVideo(){
    return Video;
}

std::array <bool,16>& Chip8::getKeys(){
    return Keys;
}

void Chip8::setPrevKeys(std::array <bool,16> Keys){
    PrevKeys=Keys;
}

//timers

void Chip8::updateTimers(){
    decrementDT();
    decrementST();
}

void Chip8::decrementDT(){
    //delay flag is set true in set DT op
    if(delay_timer>0)
        delay_timer--;
    else
        delayflag=false;
    
} 

void Chip8::decrementST(){
    //sound flag is set true in set ST op
    if(sound_timer>0) 
        sound_timer--;
    else 
        soundflag=false;
}


//operations

void Chip8::opUnknownOpcode(uint16_t opcode){
    std::cerr << "Unknown opcode: 0x"
              << std::hex << opcode
              << std::dec
              << '\n';
}

void Chip8::opClearScreen(){
    Video.fill(0);
    drawflag = true;
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
    //Quirk
    #if VF_RESET
        V[0xF]=0x0;
    #endif
}

void Chip8::opAND(uint8_t x,uint8_t y){
    V[x]&=V[y];
    //Quirk
    #if VF_RESET
        V[0xF]=0x0;
    #endif
}

void Chip8::opXOR(uint8_t x,uint8_t y){
    V[x]^=V[y];
    //Quirk
    #if VF_RESET
        V[0xF]=0x0;
    #endif
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

void Chip8::opSHR(uint8_t x ,uint8_t y){
    //Quirk
    #if !SHIFTING
        V[x]=V[y];
    #endif
    V[0xF]=V[x] & 0x01;         //sets flag to least significant bit
    V[x]>>=1;    
                       
}

void Chip8::opSubN(uint8_t x,uint8_t y){
    V[0xF]=(V[y]>=V[x]);
    V[x]=V[y]-V[x];
}

void Chip8::opSHL(uint8_t x ,uint8_t y){
    //Quirk
    #if !SHIFTING
        V[x]=V[y];
    #endif
    V[0xF]=V[x] >> 7;         //sets flag to most significant bit
    V[x]<<=1;
    
}

void Chip8::opSkipNotReg(uint8_t x,uint8_t y){
    if(V[x]!=V[y])PC+=2;
}

void Chip8::opSetI(uint16_t addr){
    I=addr;
}

void Chip8::opJumpOffset(uint16_t addr,uint8_t x){
    //Quirk
    #if JUMP_WITH_OFFSET
        PC = addr + V[x];
    #else
        PC = addr + V[0x0];
    #endif
}

void Chip8::opSetRandom(uint8_t x,uint8_t byte){
    V[x]= static_cast<uint8_t>(dist(rng)) & byte;
}

void Chip8::opDraw(uint8_t x,uint8_t y,uint8_t nibble){

    drawflag=true;
    V[0xF]=0X00;                              //initialise flag to check collision
    const uint8_t startX = V[x] % 64;
    const uint8_t startY = V[y] % 32;
    // CHIP-8: DXYN draws an N-row sprite (each row is 8 pixels).
    // Super-CHIP: DXY0 draws a 16x16 sprite (each row uses 2 bytes).
    // Some ROMs (e.g. 1dcell) rely on the DXY0 behavior.
    if(nibble == 0){
        for(int i = 0; i < 16; i++){
            const uint8_t leftByte  = Memory[I + static_cast<uint16_t>(i * 2)];
            const uint8_t rightByte = Memory[I + static_cast<uint16_t>(i * 2 + 1)];

            // Left 8 pixels
            for(int j = 0; j < 8; j++){
                const bool bit = (leftByte >> (7 - j)) & 0x01;
                if(!bit) continue;

                #if CLIPPING
                    // If the sprite pixel is outside the screen, skip it.
                    if((startX + j) >= 64 || (startY + i) >= 32) continue;
                #endif
                const uint8_t screenX = (startX + j) % 64;
                const uint8_t screenY = (startY + i) % 32;
                const uint16_t index = screenY * 64 + screenX;

                if(Video[index] && bit) V[0xF] = 0x01; // collision (pixel flipped from 1->0)
                Video[index] = Video[index] ^ bit;      // XOR draw
            }

            // Right 8 pixels
            for(int j = 0; j < 8; j++){
                const bool bit = (rightByte >> (7 - j)) & 0x01;
                if(!bit) continue;

                #if CLIPPING
                    // If the sprite pixel is outside the screen, skip it.
                    if((startX + 8 + j) >= 64 || (startY + i) >= 32) continue;
                #endif
                const uint8_t screenX = (startX + 8 + j) % 64;
                const uint8_t screenY = (startY + i) % 32;
                const uint16_t index = screenY * 64 + screenX;

                if(Video[index] && bit) V[0xF] = 0x01;
                Video[index] = Video[index] ^ bit;
            }
        }
        return;
    }

    // Standard CHIP-8: read N rows, 1 byte per row.
    for(int i = 0; i < nibble; i++){
        const uint8_t byte = Memory[I + static_cast<uint16_t>(i)];
        for(int j = 0; j < 8; j++){
            const bool bit = (byte >> (7 - j)) & 0x01; // sprite pixel at (x+j, y+i)
            if(!bit) continue; // XORing with 0 changes nothing

            #if CLIPPING
                // If the sprite pixel is outside the screen, skip it.
                if((startX + j) >= 64 || (startY + i) >= 32) continue;
            #endif

            const uint8_t screenX = (startX + j) % 64;
            const uint8_t screenY = (startY + i) % 32;

            const uint16_t index = screenY * 64 + screenX;

            if(Video[index] && bit) V[0xF] = 0x01;
            Video[index] = Video[index] ^ bit;
        }
    }
}

void Chip8::opSkipKey(uint8_t x){
    if(Keys[V[x]]==1) PC+=2;
}

void Chip8::opSkipKeyNot(uint8_t x){
    if(Keys[V[x]]==0) PC+=2;
}

void Chip8::opGetDT(uint8_t x){
    V[x]=delay_timer;
}

void Chip8::opLoadKeyPress(uint8_t x){
    // Blocking wait: if no key is down, re-fetch this opcode next cycle.
    for(uint8_t i=0x0;i<=0xF;i++){
        if(Keys[i] && !PrevKeys[i]){
            V[x]=i;
            return;
        }
    }
    PC-=2; // rewind PC so FX0A is retried until a key is pressed
}

void Chip8::opSetDT(uint8_t x){
    delay_timer=V[x];
    //helper flag
    delayflag=true;
}

void Chip8::opSetST(uint8_t x){
    sound_timer=V[x];
    //helper flag
    soundflag=true;
}

void Chip8::opAddI(uint8_t x){
    I+=V[x];
}

void Chip8::opPointSprite(uint8_t x){
    //loads pointer to sprite 0-f 
    I=FONT_START_ADDRESS+(V[x]*5);
}

void Chip8::opBCD(uint8_t x){
    uint8_t num = V[x];
    Memory[I]=num/100;
    num%=100;
    Memory[I+1]=num/10;
    num%=10;
    Memory[I+2]=num;
}

void Chip8::opStoreMem(uint8_t x){
    for(uint8_t i=0;i<=x;i++){
        //Quirk
        #if MEMORY_INC
            Memory[I]=V[i];
            I++;
        #else
            Memory[I+i]=V[i];
        #endif
    }
}

void Chip8::opLoadMem(uint8_t x){
    for(uint8_t i=0;i<=x;i++){
        //Quirk
        #if MEMORY_INC
            V[i]=Memory[I];
            I++;
        #else
            V[i]=Memory[I+i];
        #endif
    }
}
