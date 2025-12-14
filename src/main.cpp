#include "chip8.h"
#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
    // Create CHIP-8 emulator instance
    CHIP8 emulator;
    
    // Load ROM
    if (!emulator.loadROM("./assets/roms/output.ch8")) {
      cerr << "Failed to load ROM!" << endl;
        return 1;
    }
    
    // Run the emulator
    emulator.run();
    
    return 0;
}