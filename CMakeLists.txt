cmake_minimum_required(VERSION "3.29.3")
project("GBAEmulator")

set(SDL2_DIR "C:/GBAEmulator/SDL2-2.30.5/cmake")

# Specify the C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# Find SDL2 package
find_package(SDL2 REQUIRED COMPONENTS SDL2)

# Include directories
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${SDL2_INCLUDE_DIR})

# Add source files
set(SOURCES
    src/main.c
    src/sdlUtil.c
    src/cpu.c
    src/memory.c
    src/ppu.c
    src/armInstructions.c
    src/thumbInstructions.c
    src/armProc.c
    src/thumbProc.c
    src/dma.c
    src/apu.c
)

# Add the executable
add_executable("GBAEmulator" ${SOURCES})

# Link the SDL2 library
target_link_libraries(GBAEmulator ${SDL2_LIBRARIES})