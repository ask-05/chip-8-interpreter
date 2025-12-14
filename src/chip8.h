#pragma once // Ensures this header file is included only once

#include <cstdint>
#include <chrono>
#include <SDL3/SDL.h>

class CHIP8 {
private:
    // CHIP-8 Memory and Registers
    static constexpr uint16_t MEMORY_SIZE = 4096;
    static constexpr uint16_t PROGRAM_START = 0x200;
    static constexpr uint16_t FONTSET_START = 0x50;
    static constexpr uint16_t FONTSET_SIZE = 80;
    static constexpr int STACK_SIZE = 16;
    static constexpr int CHIP8_WIDTH = 64;
    static constexpr int CHIP8_HEIGHT = 32;
    
    // Clock and Timer Speeds
    static constexpr int CLOCK_SPEED = 650;
    static constexpr int TIMER_SPEED = 60;
    static constexpr bool DEBUG_OPCODES = false;

    // Memory and Registers
    uint8_t memory[MEMORY_SIZE];
    uint8_t V[16];              // V0-VF (VF is flag register)
    uint16_t I;                 // Index register
    uint16_t pc;                // Program counter
    uint16_t stack[STACK_SIZE]; // Stack for subroutine return addresses
    int sp;                     // Stack pointer
    uint8_t delay_timer;        // Delay timer
    uint8_t sound_timer;        // Sound timer
    uint8_t key[16];            // Keypad state
    uint64_t display[32];       // Display buffer (64x32 pixels)
    uint16_t opcode;            // Current opcode

    // Font data
    static constexpr uint8_t chip8_fontset[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    // Timing
    std::chrono::high_resolution_clock::time_point last_cycle_time;
    std::chrono::high_resolution_clock::time_point last_timer_update;

    // ROM loaded flag
    bool rom_loaded;

    // Opcode execution methods
    void execute_opcode();
    void incPC();
    void logOpcode(uint16_t op);
    void updateTimers();
    int genRandomNum();
    
    // Validation helpers
    bool isValidMemoryAddress(uint16_t address);
    bool isValidStackPointer();
    bool isValidKeyIndex(uint8_t key_index);

public:
    // Constructor and Destructor
    CHIP8();
    ~CHIP8();
    
    // Public interface
    bool loadROM(const char* filename);
    void run();
    void handleKeyEvent(SDL_KeyboardEvent key_event);
    
    // Getters for display and state
    const uint64_t* getDisplay() const { return display; }
    bool isRunning() const { return rom_loaded; }
};
