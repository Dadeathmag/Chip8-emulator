// Main entry point: ties together the CHIP-8 core and the SDL2 platform layer.
// The main loop uses fixed timestep accumulators so CPU speed, timers, and
// rendering can run at independent rates regardless of frame time jitter.

#include <filesystem>
#include "core/Chip8.hpp"
#include "platform/PlatformSDL2.hpp"

#define TITLE "CHIP-8 Emulator"
#define SCREENWIDTH 64   // native CHIP-8 display width in pixels
#define SCREENHEIGHT 32  // native CHIP-8 display height in pixels
#define DEFAULT_SCALE 8  // initial window upscale factor

namespace {
    std::string romDisplayName(const std::string& path){
        return std::filesystem::path(path).filename().string();
    }

    // Picks the first .ch8 file found in roms/ (directory iteration order).
    std::string findDefaultRom(){
        namespace fs = std::filesystem;
        if(!fs::exists("roms") || !fs::is_directory("roms")){
            return {};
        }

        for(const auto& entry : fs::directory_iterator("roms")){
            if(entry.is_regular_file() && entry.path().extension() == ".ch8"){
                return entry.path().string();
            }
        }
        return {};
    }
}

#if defined(__WIN32__) && !defined(__EMSCRIPTEN__)
extern "C"
#endif
int main(int /*argc*/, char* /*argv*/[])
{
    PlatformSDL2 sdl(TITLE, 100, 100, SCREENWIDTH, SCREENHEIGHT, DEFAULT_SCALE);

    Chip8 chip8;
    std::string currentRom = findDefaultRom();
    if(!currentRom.empty()){
        chip8.loadROM(currentRom.c_str());
        sdl.setRomLabel(romDisplayName(currentRom));
    }else{
        sdl.setRomLabel("NO ROM LOADED");
    }

    // Accumulators track how much "work" is owed since the last frame.
    // CPU: 1000 units = one opcode cycle at the configured Hz.
    // Timers: CHIP-8 delay/sound timers tick at 60 Hz.
    // Render: capped at ~60 FPS (16 ms per frame).
    uint32_t cpuaccumulator = 0;
    uint32_t timeraccumulator = 0;
    uint32_t renderaccumulator = 0;
    uint32_t lasttime = sdl.getTicks();

    while(sdl.isRunning()){
        uint32_t currenttime = sdl.getTicks();
        uint32_t deltatime = currenttime - lasttime;
        lasttime = currenttime;

        chip8.cpufrequency = sdl.getCpuFrequency();
        cpuaccumulator += deltatime * static_cast<uint32_t>(chip8.cpufrequency);
        timeraccumulator += deltatime * 60;

        sdl.handleInput(chip8.getKeys()); //debouncing to implement

        // UI actions are queued by the platform layer and consumed here so
        // ROM loading/resetting never happens mid-input handling.
        UiCommand command;
        if(sdl.pollUiCommand(command)){
            if(command.type == UiCommand::Type::LoadRom){
                currentRom = command.romPath;
                chip8.loadROM(currentRom.c_str());
                sdl.setRomLabel(romDisplayName(currentRom));
            }else if(command.type == UiCommand::Type::Reset && !currentRom.empty()){
                chip8.loadROM(currentRom.c_str());
            }else if(command.type == UiCommand::Type::DebugToggled){
                // Hook for future debug panel / logging
            }
        }

        // Run as many CPU cycles as the elapsed time allows.
        while(cpuaccumulator >= 1000){
            chip8.cycle();
            cpuaccumulator -= 1000;
            if(chip8.drawflag){
                chip8.drawflag = false; // draw flag consumed; screen updated on render tick
            }
        }

        // Delay and sound timers always run at 60 Hz on real hardware.
        while(timeraccumulator >= 1000){
            chip8.updateTimers();
            timeraccumulator -= 1000;
        }

        sdl.makeBeep(chip8.soundflag);

        renderaccumulator += deltatime;
        if(renderaccumulator >= 16){ // ~60 FPS display refresh
            sdl.updateScreen(chip8.getVideo());
            renderaccumulator -= 16;
        }
    }

    return 0;
}
