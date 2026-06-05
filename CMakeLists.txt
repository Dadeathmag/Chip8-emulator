cmake_minimum_required(VERSION 3.16)

#defines name and version of the project, as well as the programming language used
project(
    Chip8
    VERSION 1.0
    LANGUAGES CXX
)

#sets the C++ standard to C++20, and ensures that it is required and that no compiler-specific extensions are used
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#finds the SDL2 library and makes it available for use in the project. If SDL2 is not found, an error will be raised
find_package(SDL2 REQUIRED)

#creates an executable target named "chip8" and specifies the source files that will be compiled to create the executable
#used for gcc chip8 src/main.cpp src/Chip8.cpp src/Platform.cpp -o chip8 -lSDL2
add_executable(
    chip8
    src/main.cpp
    src/Chip8.cpp
    src/Platform.cpp
)

#specifies that the "src" directory should be included in the search path for header files when compiling the "chip8" target
target_include_directories(
    chip8
    PRIVATE
    src
)

#specifies the libraries that the "chip8" target should link against
target_link_libraries(
    chip8
    PRIVATE
    SDL2::SDL2
)


target_compile_options(
    chip8
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

#configure and build this file using
#cmake -B build
#cmake --build build