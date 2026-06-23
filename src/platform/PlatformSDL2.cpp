// SDL2 platform layer: rendering, input, audio, and toolbar UI.
// CHIP-8 keypad layout mapped to keyboard:
//   1 2 3 4       1 2 3 C
//   Q W E R  =>   4 5 6 D
//   A S D F       7 8 9 E
//   Z X C V       A 0 B F

#include "PlatformSDL2.hpp"
#include "BitmapFont.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

namespace {
    constexpr int BEEP_FREQUENCY = 440;
    constexpr int BEEP_AMPLITUDE = 2000;
    constexpr uint32_t BAR_BG = 0xFF1E1E24;
    constexpr uint32_t BAR_BORDER = 0xFF3A3A48;
    constexpr uint32_t BTN_BG = 0xFF2D2D38;
    constexpr uint32_t BTN_HOVER = 0xFF45455A;
    constexpr uint32_t BTN_TEXT = 0xFFE8E8F0;
    constexpr uint32_t OVERLAY_BG = 0xE0181820;
    constexpr uint32_t PICKER_TEXT = 0xFFE0E0E8;
    constexpr uint32_t PICKER_HILITE = 0xFF4A6FA5;
    constexpr uint32_t SCROLL_TRACK = 0xFF2A2A34;
    constexpr uint32_t SCROLL_THUMB = 0xFF5A5A70;
    constexpr int ROM_PICKER_ROW_H = 28;
}

// Returns CHIP-8 keypad index (0x0–0xF), or -1 if unmapped.
int PlatformSDL2::mapKey(SDL_Keycode key){
    switch(key){
        case SDLK_1: return 0x1;
        case SDLK_2: return 0x2;
        case SDLK_3: return 0x3;
        case SDLK_4: return 0xC;
        case SDLK_q: return 0x4;
        case SDLK_w: return 0x5;
        case SDLK_e: return 0x6;
        case SDLK_r: return 0xD;
        case SDLK_a: return 0x7;
        case SDLK_s: return 0x8;
        case SDLK_d: return 0x9;
        case SDLK_f: return 0xE;
        case SDLK_z: return 0xA;
        case SDLK_x: return 0x0;
        case SDLK_c: return 0xB;
        case SDLK_v: return 0xF;
        default: return -1;
    }
}

// Square-wave beep driven by the sound timer flag from the CPU.
void PlatformSDL2::audioCallback(void* userdata, Uint8* stream, int len){
    auto* platform = static_cast<PlatformSDL2*>(userdata);
    auto* buffer = reinterpret_cast<Sint16*>(stream);
    const int samples = len / static_cast<int>(sizeof(Sint16));
    const int halfPeriod = 44100 / BEEP_FREQUENCY / 2;

    for(int i = 0; i < samples; ++i){
        if(platform->beepActive){
            buffer[i] = (platform->beepPhase < halfPeriod) ? BEEP_AMPLITUDE : -BEEP_AMPLITUDE;
            platform->beepPhase = (platform->beepPhase + 1) % (halfPeriod * 2);
        }else{
            buffer[i] = 0;
        }
    }
}

PlatformSDL2::PlatformSDL2(const char *title, int x, int y, int screen_width, int screen_height, int initial_scale)
    : running(true)
    , screenWidth(screen_width)
    , screenHeight(screen_height)
{
    for(std::size_t i = 0; i < GAME_SCALES.size(); ++i){
        if(GAME_SCALES[i] == initial_scale){
            scaleIndex = static_cast<int>(i);
            break;
        }
    }
    gameScale = GAME_SCALES[scaleIndex];
    windowWidth = screenWidth * gameScale;
    windowHeight = BAR_HEIGHT + screenHeight * gameScale;

    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow(title, x, y, windowWidth, windowHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, screen_width, screen_height);
    Pixels.fill(0);

    if(!window){
        std::cout << "Failed to create a window! Error: " << SDL_GetError() << std::endl;
    }

    layoutToolbar();
    scanRoms();

    SDL_AudioSpec want{};
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 256;
    want.callback = audioCallback;
    want.userdata = this;

    SDL_AudioSpec have{};
    audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if(audioDevice == 0){
        std::cout << "Failed to open audio device! Error: " << SDL_GetError() << std::endl;
    }else{
        SDL_PauseAudioDevice(audioDevice, 0);
    }
}

PlatformSDL2::~PlatformSDL2(){
    if(audioDevice != 0){
        SDL_CloseAudioDevice(audioDevice);
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    window = nullptr;
    renderer = nullptr;
    texture = nullptr;
    SDL_Quit();
}

// Re-read roms/ and sort alphabetically for the picker list.
void PlatformSDL2::scanRoms(){
    romFiles.clear();
    namespace fs = std::filesystem;

    if(fs::exists("roms") && fs::is_directory("roms")){
        for(const auto& entry : fs::directory_iterator("roms")){
            if(!entry.is_regular_file()) continue;
            const auto path = entry.path();
            if(path.extension() == ".ch8"){
                romFiles.push_back(path.string());
            }
        }
    }

    std::sort(romFiles.begin(), romFiles.end());
}

// Position toolbar controls; romLabelRect fills remaining width on the right.
void PlatformSDL2::layoutToolbar(){
    const int pad = 4;
    const int btnH = BAR_HEIGHT - pad * 2;
    int x = pad;

    auto place = [&](int index, int width, const char* label){
        toolbarButtons[index] = Button{{x, pad, width, btnH}, label};
        x += width + pad;
    };

    place(0, 60, "LOAD");
    place(1, 50, "RESET");

    cpuInputRect = {x, pad, 58, btnH};
    x += 58 + pad;
    place(2, 32, "SET");
    place(3, 54, "PAL");
    place(4, 42, "DBG");
    place(5, 22, "-");
    scaleLabelRect = {x, pad, 34, btnH};
    x += 34 + pad;
    place(6, 22, "+");

    x += pad;
    romLabelRect = {x, pad, std::max(60, windowWidth - x - pad), btnH};
}

// Resize the window so each CHIP-8 pixel is drawn at `scale` screen pixels.
void PlatformSDL2::applyGameScale(int scale){
    gameScale = scale;
    windowWidth = screenWidth * gameScale;
    windowHeight = BAR_HEIGHT + screenHeight * gameScale;
    SDL_SetWindowSize(window, windowWidth, windowHeight);
    layoutToolbar();
}

bool PlatformSDL2::pointInRect(int px, int py, const SDL_Rect& rect) const{
    return px >= rect.x && px < rect.x + rect.w
        && py >= rect.y && py < rect.y + rect.h;
}

void PlatformSDL2::drawButton(const Button& button, bool highlight){
    const uint32_t bg = highlight ? BTN_HOVER : BTN_BG;
    SDL_SetRenderDrawColor(renderer, (bg >> 16) & 0xFF, (bg >> 8) & 0xFF, bg & 0xFF, 255);
    SDL_RenderFillRect(renderer, &button.rect);
    SDL_SetRenderDrawColor(renderer, (BAR_BORDER >> 16) & 0xFF, (BAR_BORDER >> 8) & 0xFF, BAR_BORDER & 0xFF, 255);
    SDL_RenderDrawRect(renderer, &button.rect);

    const int textW = static_cast<int>(std::strlen(button.label)) * 6;
    const int textX = button.rect.x + (button.rect.w - textW) / 2;
    const int textY = button.rect.y + (button.rect.h - 7) / 2;
    drawText(renderer, textX, textY, button.label, BTN_TEXT);
}

void PlatformSDL2::drawCpuInput(){
    const uint32_t bg = cpuInputFocused ? BTN_HOVER : BTN_BG;
    SDL_SetRenderDrawColor(renderer, (bg >> 16) & 0xFF, (bg >> 8) & 0xFF, bg & 0xFF, 255);
    SDL_RenderFillRect(renderer, &cpuInputRect);
    SDL_SetRenderDrawColor(renderer, (BAR_BORDER >> 16) & 0xFF, (BAR_BORDER >> 8) & 0xFF, BAR_BORDER & 0xFF, 255);
    SDL_RenderDrawRect(renderer, &cpuInputRect);

    drawText(renderer, cpuInputRect.x + 4, cpuInputRect.y + 4, "CPU", BTN_TEXT);

    std::string valueText = cpuInputText;
    if(cpuInputFocused && (SDL_GetTicks() / 500) % 2 == 0){
        valueText += '_';
    }
    drawText(renderer, cpuInputRect.x + 4, cpuInputRect.y + 18, valueText.c_str(), BTN_TEXT);
}

void PlatformSDL2::drawPaletteButton(bool highlight){
    const auto& button = toolbarButtons[3];
    const uint32_t bg = highlight ? BTN_HOVER : BTN_BG;
    SDL_SetRenderDrawColor(renderer, (bg >> 16) & 0xFF, (bg >> 8) & 0xFF, bg & 0xFF, 255);
    SDL_RenderFillRect(renderer, &button.rect);
    SDL_SetRenderDrawColor(renderer, (BAR_BORDER >> 16) & 0xFF, (BAR_BORDER >> 8) & 0xFF, BAR_BORDER & 0xFF, 255);
    SDL_RenderDrawRect(renderer, &button.rect);

    drawText(renderer, button.rect.x + 22, button.rect.y + 4, "PAL", BTN_TEXT);

    const auto& palette = PALETTES[paletteIndex];
    SDL_Rect swatch{button.rect.x + 32, button.rect.y + 16, 12, 12};
    SDL_SetRenderDrawColor(renderer, (palette.off >> 16) & 0xFF, (palette.off >> 8) & 0xFF, palette.off & 0xFF, 255);
    SDL_RenderFillRect(renderer, &swatch);
    SDL_SetRenderDrawColor(renderer, (palette.on >> 16) & 0xFF, (palette.on >> 8) & 0xFF, palette.on & 0xFF, 255);
    SDL_Rect onPx{swatch.x + 3, swatch.y + 3, 6, 6};
    SDL_RenderFillRect(renderer, &onPx);
}

void PlatformSDL2::drawDebugButton(bool highlight){
    const auto& button = toolbarButtons[4];
    const uint32_t bg = (highlight || debugMode) ? BTN_HOVER : BTN_BG;
    SDL_SetRenderDrawColor(renderer, (bg >> 16) & 0xFF, (bg >> 8) & 0xFF, bg & 0xFF, 255);
    SDL_RenderFillRect(renderer, &button.rect);
    SDL_SetRenderDrawColor(renderer, (BAR_BORDER >> 16) & 0xFF, (BAR_BORDER >> 8) & 0xFF, BAR_BORDER & 0xFF, 255);
    SDL_RenderDrawRect(renderer, &button.rect);

    const uint32_t textColor = debugMode ? 0xFF88CCFF : BTN_TEXT;
    drawText(renderer, button.rect.x + 6, button.rect.y + 12, "DBG", textColor);
}

void PlatformSDL2::drawScaleControls(){
    int mouseX = 0;
    int mouseY = 0;
    SDL_GetMouseState(&mouseX, &mouseY);

    drawButton(toolbarButtons[5], pointInRect(mouseX, mouseY, toolbarButtons[5].rect));

    SDL_SetRenderDrawColor(renderer, (BTN_BG >> 16) & 0xFF, (BTN_BG >> 8) & 0xFF, BTN_BG & 0xFF, 255);
    SDL_RenderFillRect(renderer, &scaleLabelRect);
    SDL_SetRenderDrawColor(renderer, (BAR_BORDER >> 16) & 0xFF, (BAR_BORDER >> 8) & 0xFF, BAR_BORDER & 0xFF, 255);
    SDL_RenderDrawRect(renderer, &scaleLabelRect);

    char scaleText[8];
    std::snprintf(scaleText, sizeof(scaleText), "%dX", gameScale);
    const int textW = static_cast<int>(std::strlen(scaleText)) * 6;
    const int textX = scaleLabelRect.x + (scaleLabelRect.w - textW) / 2;
    drawText(renderer, textX, scaleLabelRect.y + 12, scaleText, BTN_TEXT);

    drawButton(toolbarButtons[6], pointInRect(mouseX, mouseY, toolbarButtons[6].rect));
}

void PlatformSDL2::drawDebugOverlay(){
    SDL_Rect banner{8, BAR_HEIGHT + 8, 120, 18};
    SDL_SetRenderDrawColor(renderer, 30, 30, 40, 200);
    SDL_RenderFillRect(renderer, &banner);
    SDL_SetRenderDrawColor(renderer, 100, 140, 200, 255);
    SDL_RenderDrawRect(renderer, &banner);
    drawText(renderer, banner.x + 6, banner.y + 5, "DEBUG ON", 0xFF88CCFF);
}

void PlatformSDL2::drawRomLabel(){
    SDL_SetRenderDrawColor(renderer, (BTN_BG >> 16) & 0xFF, (BTN_BG >> 8) & 0xFF, BTN_BG & 0xFF, 255);
    SDL_RenderFillRect(renderer, &romLabelRect);
    SDL_SetRenderDrawColor(renderer, (BAR_BORDER >> 16) & 0xFF, (BAR_BORDER >> 8) & 0xFF, BAR_BORDER & 0xFF, 255);
    SDL_RenderDrawRect(renderer, &romLabelRect);

    const int maxChars = std::max(1, romLabelRect.w / 6);
    std::string label = romLabel;
    if(static_cast<int>(label.size()) > maxChars){
        label = "..." + label.substr(label.size() - static_cast<std::size_t>(maxChars - 3));
    }

    const int textW = static_cast<int>(label.size()) * 6;
    const int textX = romLabelRect.x + romLabelRect.w - textW - 4;
    const int textY = romLabelRect.y + (romLabelRect.h - 7) / 2;
    drawText(renderer, textX, textY, label.c_str(), BTN_TEXT);
}

void PlatformSDL2::drawToolbar(){
    SDL_SetRenderDrawColor(renderer, (BAR_BG >> 16) & 0xFF, (BAR_BG >> 8) & 0xFF, BAR_BG & 0xFF, 255);
    SDL_Rect bar{0, 0, windowWidth, BAR_HEIGHT};
    SDL_RenderFillRect(renderer, &bar);

    int mouseX = 0;
    int mouseY = 0;
    SDL_GetMouseState(&mouseX, &mouseY);

    drawButton(toolbarButtons[0], pointInRect(mouseX, mouseY, toolbarButtons[0].rect));
    drawButton(toolbarButtons[1], pointInRect(mouseX, mouseY, toolbarButtons[1].rect));
    drawCpuInput();
    drawButton(toolbarButtons[2], pointInRect(mouseX, mouseY, toolbarButtons[2].rect));
    drawPaletteButton(pointInRect(mouseX, mouseY, toolbarButtons[3].rect));
    drawDebugButton(pointInRect(mouseX, mouseY, toolbarButtons[4].rect));
    drawScaleControls();
    drawRomLabel();

    SDL_SetRenderDrawColor(renderer, (BAR_BORDER >> 16) & 0xFF, (BAR_BORDER >> 8) & 0xFF, BAR_BORDER & 0xFF, 255);
    SDL_RenderDrawLine(renderer, 0, BAR_HEIGHT - 1, windowWidth, BAR_HEIGHT - 1);
}

SDL_Rect PlatformSDL2::romPickerListRect() const{
    return SDL_Rect{
        16,
        BAR_HEIGHT + 56,
        windowWidth - 32,
        windowHeight - BAR_HEIGHT - 64
    };
}

int PlatformSDL2::maxRomPickerScroll(int rowH, int listHeight) const{
    const int contentH = static_cast<int>(romFiles.size()) * rowH;
    return std::max(0, contentH - listHeight);
}

void PlatformSDL2::drawRomPickerScrollbar(const SDL_Rect& listRect, int rowH, int contentH){
    const int maxScroll = maxRomPickerScroll(rowH, listRect.h);
    if(maxScroll <= 0){
        return;
    }

    const int trackX = listRect.x + listRect.w - 10;
    SDL_Rect track{trackX, listRect.y, 6, listRect.h};
    SDL_SetRenderDrawColor(renderer, (SCROLL_TRACK >> 16) & 0xFF, (SCROLL_TRACK >> 8) & 0xFF, SCROLL_TRACK & 0xFF, 255);
    SDL_RenderFillRect(renderer, &track);

    const int thumbH = std::max(20, (listRect.h * listRect.h) / contentH);
    const int thumbY = listRect.y + (romPickerScroll * (listRect.h - thumbH)) / maxScroll;
    SDL_Rect thumb{trackX, thumbY, 6, thumbH};
    SDL_SetRenderDrawColor(renderer, (SCROLL_THUMB >> 16) & 0xFF, (SCROLL_THUMB >> 8) & 0xFF, SCROLL_THUMB & 0xFF, 255);
    SDL_RenderFillRect(renderer, &thumb);
}

void PlatformSDL2::drawRomPicker(){
    SDL_SetRenderDrawColor(renderer, (OVERLAY_BG >> 16) & 0xFF, (OVERLAY_BG >> 8) & 0xFF, OVERLAY_BG & 0xFF, OVERLAY_BG >> 24);
    SDL_Rect overlay{0, BAR_HEIGHT, windowWidth, windowHeight - BAR_HEIGHT};
    SDL_RenderFillRect(renderer, &overlay);

    drawText(renderer, 24, BAR_HEIGHT + 16, "SELECT ROM", PICKER_TEXT, 2);

    if(romFiles.empty()){
        drawText(renderer, 24, BAR_HEIGHT + 56, "NO .CH8 FILES IN ROMS/", PICKER_TEXT, 2);
        return;
    }

    const SDL_Rect listRect = romPickerListRect();
    const int contentH = static_cast<int>(romFiles.size()) * ROM_PICKER_ROW_H;
    romPickerScroll = std::min(romPickerScroll, maxRomPickerScroll(ROM_PICKER_ROW_H, listRect.h));

    int mouseX = 0;
    int mouseY = 0;
    SDL_GetMouseState(&mouseX, &mouseY);

    SDL_RenderSetClipRect(renderer, &listRect);
    for(std::size_t i = 0; i < romFiles.size(); ++i){
        const int rowY = listRect.y + static_cast<int>(i) * ROM_PICKER_ROW_H - romPickerScroll;
        if(rowY + ROM_PICKER_ROW_H < listRect.y || rowY > listRect.y + listRect.h){
            continue;
        }

        const std::string name = std::filesystem::path(romFiles[i]).filename().string();
        SDL_Rect row{listRect.x, rowY, listRect.w - 12, ROM_PICKER_ROW_H - 4};
        if(pointInRect(mouseX, mouseY, row)){
            SDL_SetRenderDrawColor(renderer, (PICKER_HILITE >> 16) & 0xFF, (PICKER_HILITE >> 8) & 0xFF, PICKER_HILITE & 0xFF, 255);
            SDL_RenderFillRect(renderer, &row);
        }

        drawText(renderer, row.x + 8, row.y + 8, name.c_str(), PICKER_TEXT);
    }
    SDL_RenderSetClipRect(renderer, nullptr);

    drawRomPickerScrollbar(listRect, ROM_PICKER_ROW_H, contentH);
}

// Returns true if the click was handled (toolbar area); false if below the bar.
bool PlatformSDL2::handleToolbarClick(int x, int y){
    if(y >= BAR_HEIGHT){
        return false;
    }

    cpuInputFocused = false;

    if(pointInRect(x, y, toolbarButtons[0].rect)){
        scanRoms();
        romPickerScroll = 0;
        showRomPicker = true;
        return true;
    }
    if(pointInRect(x, y, toolbarButtons[1].rect)){
        resetRequested = true;
        return true;
    }
    if(pointInRect(x, y, cpuInputRect)){
        cpuInputFocused = true;
        return true;
    }
    if(pointInRect(x, y, toolbarButtons[2].rect)){
        commitCpuInput();
        return true;
    }
    if(pointInRect(x, y, toolbarButtons[3].rect)){
        paletteIndex = (paletteIndex + 1) % static_cast<int>(PALETTES.size());
        return true;
    }
    if(pointInRect(x, y, toolbarButtons[4].rect)){
        debugMode = !debugMode;
        debugToggleRequested = true;
        return true;
    }
    if(pointInRect(x, y, toolbarButtons[5].rect)){
        scaleIndex = std::max(0, scaleIndex - 1);
        applyGameScale(GAME_SCALES[scaleIndex]);
        return true;
    }
    if(pointInRect(x, y, toolbarButtons[6].rect)){
        scaleIndex = std::min(static_cast<int>(GAME_SCALES.size()) - 1, scaleIndex + 1);
        applyGameScale(GAME_SCALES[scaleIndex]);
        return true;
    }
    if(pointInRect(x, y, scaleLabelRect)){
        scaleIndex = (scaleIndex + 1) % static_cast<int>(GAME_SCALES.size());
        applyGameScale(GAME_SCALES[scaleIndex]);
        return true;
    }
    if(pointInRect(x, y, romLabelRect)){
        return true;
    }

    return true;
}

// Parse and clamp the CPU frequency field, then sync the displayed text.
void PlatformSDL2::commitCpuInput(){
    if(cpuInputText.empty()){
        cpuInputText = std::to_string(cpuFrequency);
        return;
    }

    try{
        const long value = std::stol(cpuInputText);
        cpuFrequency = static_cast<int>(std::clamp(value, 100L, 10000L));
    }catch(...){
        cpuFrequency = 1000;
    }

    cpuInputText = std::to_string(cpuFrequency);
    cpuInputFocused = false;
}

bool PlatformSDL2::handleRomPickerClick(int x, int y){
    if(y < BAR_HEIGHT){
        showRomPicker = false;
        return true;
    }

    const SDL_Rect listRect = romPickerListRect();
    if(!pointInRect(x, y, listRect)){
        showRomPicker = false;
        return true;
    }

    const int relativeY = y - listRect.y + romPickerScroll;
    const int index = relativeY / ROM_PICKER_ROW_H;
    if(index >= 0 && index < static_cast<int>(romFiles.size())){
        pendingRomPath = romFiles[static_cast<std::size_t>(index)];
        showRomPicker = false;
    }

    return true;
}

// Poll SDL events: toolbar clicks, ROM picker, CPU input, and CHIP-8 keys.
void PlatformSDL2::handleInput(std::array <bool,16>& keys){
    while(SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_QUIT:
                running = false;
                break;

            case SDL_MOUSEBUTTONDOWN:
                if(event.button.button == SDL_BUTTON_LEFT){
                    if(showRomPicker){
                        handleRomPickerClick(event.button.x, event.button.y);
                    }else{
                        handleToolbarClick(event.button.x, event.button.y);
                    }
                }
                break;

            case SDL_MOUSEWHEEL:
                if(showRomPicker && !romFiles.empty()){
                    const SDL_Rect listRect = romPickerListRect();
                    romPickerScroll -= event.wheel.y * ROM_PICKER_ROW_H;
                    romPickerScroll = std::clamp(
                        romPickerScroll,
                        0,
                        maxRomPickerScroll(ROM_PICKER_ROW_H, listRect.h)
                    );
                }
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                if(cpuInputFocused && event.type == SDL_KEYDOWN){
                    switch(event.key.keysym.sym){
                        case SDLK_RETURN:
                        case SDLK_KP_ENTER:
                            commitCpuInput();
                            break;
                        case SDLK_ESCAPE:
                            cpuInputText = std::to_string(cpuFrequency);
                            cpuInputFocused = false;
                            break;
                        case SDLK_BACKSPACE:
                            if(!cpuInputText.empty()){
                                cpuInputText.pop_back();
                            }
                            break;
                        default: {
                            if(cpuInputText.size() >= 5) break;
                            const SDL_Keycode sym = event.key.keysym.sym;
                            if(sym >= SDLK_0 && sym <= SDLK_9){
                                cpuInputText += static_cast<char>('0' + (sym - SDLK_0));
                            }else if(sym >= SDLK_KP_0 && sym <= SDLK_KP_9){
                                cpuInputText += static_cast<char>('0' + (sym - SDLK_KP_0));
                            }
                            break;
                        }
                    }
                    break;
                }

                const int chipKey = mapKey(event.key.keysym.sym);
                if(chipKey >= 0){
                    keys[chipKey] = (event.type == SDL_KEYDOWN);
                }
                if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE && showRomPicker){
                    showRomPicker = false;
                }
                if(event.type == SDL_KEYDOWN && showRomPicker && !cpuInputFocused){
                    const SDL_Rect listRect = romPickerListRect();
                    const int maxScroll = maxRomPickerScroll(ROM_PICKER_ROW_H, listRect.h);
                    
                    if(event.key.keysym.sym == SDLK_UP){
                        romPickerScroll = std::max(0, romPickerScroll - ROM_PICKER_ROW_H);
                    }else if(event.key.keysym.sym == SDLK_DOWN){
                        romPickerScroll = std::min(maxScroll, romPickerScroll + ROM_PICKER_ROW_H);
                    }
                }
                break;
            }

            case SDL_WINDOWEVENT:
                // Prevent stuck keys when the window loses focus.
                if(event.window.event == SDL_WINDOWEVENT_FOCUS_LOST){
                    keys.fill(false);
                }
                break;
        }
    }
}

// Convert the monochrome display buffer to ARGB, draw toolbar, and present.
void PlatformSDL2::updateScreen(std::array <bool,64*32> Video){
    const auto& palette = PALETTES[paletteIndex];
    for(int i = 0; i < 64 * 32; i++){
        Pixels[i] = Video[i] ? palette.on : palette.off;
    }

    SDL_UpdateTexture(texture, nullptr, Pixels.data(), screenWidth * static_cast<int>(sizeof(uint32_t)));

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    drawToolbar();

    const int gameHeight = windowHeight - BAR_HEIGHT;
    SDL_Rect dest{0, BAR_HEIGHT, windowWidth, gameHeight};
    SDL_RenderCopy(renderer, texture, nullptr, &dest);

    if(showRomPicker){
        drawRomPicker();
    }

    if(debugMode){
        drawDebugOverlay();
    }

    SDL_RenderPresent(renderer);
}

void PlatformSDL2::createDelay(int value){
    SDL_Delay(value);
}

void PlatformSDL2::makeBeep(bool active){
    beepActive = active;
    if(!active){
        beepPhase = 0;
    }
}

// Drain one pending UI action per call (ROM load has highest priority).
bool PlatformSDL2::pollUiCommand(UiCommand& command){
    command.type = UiCommand::Type::None;
    command.romPath.clear();

    if(!pendingRomPath.empty()){
        command.type = UiCommand::Type::LoadRom;
        command.romPath = pendingRomPath;
        pendingRomPath.clear();
        return true;
    }

    if(resetRequested){
        command.type = UiCommand::Type::Reset;
        resetRequested = false;
        return true;
    }

    if(debugToggleRequested){
        command.type = UiCommand::Type::DebugToggled;
        debugToggleRequested = false;
        return true;
    }

    return false;
}

int PlatformSDL2::getCpuFrequency() const{
    return cpuFrequency;
}

int PlatformSDL2::getGameScale() const{
    return gameScale;
}

bool PlatformSDL2::isDebugMode() const{
    return debugMode;
}

void PlatformSDL2::setRomLabel(const std::string& label){
    romLabel = label;
}

bool PlatformSDL2::isRunning(){ return running; }
uint32_t PlatformSDL2::getTicks(){ return SDL_GetTicks(); }
