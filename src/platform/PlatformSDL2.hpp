#pragma once

#include <iostream>
#include <array>
#include <string>
#include <vector>
#include "SdlInclude.hpp"

// Deferred action from the SDL event loop to the main emulation loop.
struct UiCommand {
    enum class Type { None, Reset, LoadRom, DebugToggled };
    Type type = Type::None;
    std::string romPath; // only valid when type == LoadRom
};

// SDL2 front end: window, rendering, audio beep, keyboard input, and toolbar UI.
class PlatformSDL2{

    public:

        static constexpr int BAR_HEIGHT = 44; // pixels reserved for the toolbar above the game

        PlatformSDL2(const char *title, int x, int y, int screen_width, int screen_height, int initial_scale = 8);
        ~PlatformSDL2();
        void handleInput(std::array <bool,16>& keys);
        void updateScreen(std::array <bool,64*32> Video);
        void createDelay(int value);
        void makeBeep(bool active);

        bool pollUiCommand(UiCommand& command);
        int getCpuFrequency() const;
        int getGameScale() const;
        bool isDebugMode() const;
        void setRomLabel(const std::string& label);

        bool isRunning();
        uint32_t getTicks();

    private:

        struct Palette {
            const char* name;
            uint32_t on;
            uint32_t off;
        };

        struct Button {
            SDL_Rect rect{};
            const char* label{};
        };

        bool running;
        bool showRomPicker{};          // ROM list overlay visible
        bool resetRequested{};         // consumed by pollUiCommand
        bool debugToggleRequested{};   // consumed by pollUiCommand
        bool debugMode{};
        bool cpuInputFocused{};        // typing into the CPU frequency field
        std::string pendingRomPath;    // ROM chosen in picker, not yet loaded
        std::string romLabel{"NO ROM"};
        std::string cpuInputText{"1000"};
        int cpuFrequency{1000};        // Hz, clamped to 100–10000 on commit
        int paletteIndex{};
        int scaleIndex{3};             // index into GAME_SCALES (default 8x)
        int gameScale{8};
        int romPickerScroll{};         // vertical scroll offset in the ROM list

        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_Texture* texture;
        SDL_Event event;
        SDL_AudioDeviceID audioDevice{};
        bool beepActive{};
        int beepPhase{};
        int windowWidth{};
        int windowHeight{};
        int screenWidth{};
        int screenHeight{};
        SDL_Rect cpuInputRect{};
        SDL_Rect scaleLabelRect{};
        SDL_Rect romLabelRect{};
        std::array <uint32_t,64*32> Pixels{}; // ARGB8888 framebuffer uploaded to the texture
        std::vector<std::string> romFiles;    // paths scanned from roms/
        std::array<Button, 7> toolbarButtons{}; // LOAD, RESET, SET, PAL, DBG, -, +

        static constexpr std::array<int, 6> GAME_SCALES{{8, 10, 12, 16, 20, 24}};

        static constexpr std::array<Palette, 5> PALETTES{{
            {"AMBER",  0xFF683D12, 0xFFEC9034},
            {"GREEN",  0xFF33FF33, 0xFF0A2E0A},
            {"MONO",   0xFFFFFFFF, 0xFF000000},
            {"LCD",    0xFF4488FF, 0xFF1A1A2E},
            {"PINK",   0xFFFF4488, 0xFF2E1A28},
        }};

        void scanRoms();
        void layoutToolbar();
        void drawToolbar();
        void drawRomPicker();
        void drawRomPickerScrollbar(const SDL_Rect& listRect, int rowH, int contentH);
        int maxRomPickerScroll(int rowH, int listHeight) const;
        SDL_Rect romPickerListRect() const;
        void drawButton(const Button& button, bool highlight = false);
        void drawCpuInput();
        void drawPaletteButton(bool highlight);
        void drawDebugButton(bool highlight);
        void drawScaleControls();
        void drawRomLabel();
        void drawDebugOverlay();
        void applyGameScale(int scale);
        void commitCpuInput();
        bool handleToolbarClick(int x, int y);
        bool handleRomPickerClick(int x, int y);
        bool pointInRect(int x, int y, const SDL_Rect& rect) const;

        static void audioCallback(void* userdata, Uint8* stream, int len);
        static int mapKey(SDL_Keycode key);
};
