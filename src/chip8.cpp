#include "chip8.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <iomanip>
#include <ctime>
#include <sstream>
#ifdef _WIN32
    #include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
    #include <sys/stat.h>
    #define MKDIR(path) mkdir(path, 0755)
#endif

using namespace std;
using namespace chrono;

// Get current timestamp for logging (format: YYYY-MM-DD_HH-MM-SS_mmm)
string CHIP8::getCurrentTimestamp() {
    auto now = system_clock::now(); // std::chrono::system_clock::time_point object
    auto time = system_clock::to_time_t(now); // converts into time_t integer
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000; // gets milliseconds since epoch
    
    stringstream ss;
    #ifdef _WIN32
        struct tm timeinfo;
        localtime_s(&timeinfo, &time); // fills timeinfo with current time data
        ss << put_time(&timeinfo, "%Y-%m-%d_%H-%M-%S");
    #else
        ss << put_time(localtime(&time), "%Y-%m-%d_%H-%M-%S");
    #endif
    ss << "_" << setfill('0') << setw(3) << ms.count();
    
    return ss.str();
}

// Create directory if it doesn't exist (cross-platform)
bool CHIP8::createDirectory(const string& path) {
    try {
        // Check if directory exists by trying to open a file in it
        string test_file = path + "/.test";
        ofstream test(test_file);
        if (test.good()) {
            test.close();
            // Directory exists, delete test file
            remove(test_file.c_str());
            return true;
        }
        test.close();
        
        // Directory doesn't exist, try to create it
        if (MKDIR(path.c_str()) == 0) {
            return true;
        }
        return false;
    } 
    catch (const exception& e) {
        cerr << "Error creating directory: " << e.what() << endl;
        return false;
    }
}

// Initialize logging system
void CHIP8::initializeLogging() {
    log_directory = "./logs";
    
    // Create logs directory if it doesn't exist
    if (!createDirectory(log_directory)) {
        cerr << "Warning: Could not create logs directory" << endl;
        return;
    }
 
    // Create log filename with current timestamp
    string timestamp = getCurrentTimestamp();
    log_filename = log_directory + "/" + timestamp + ".txt";
    
    // Open log file for appending
    log_file.open(log_filename, ios::app);
    
    if (!log_file.is_open()) {
        cerr << "Warning: Could not open log file: " << log_filename << endl;
        return;
    }
    
    // Write header to log file
    writeToLog("========================================");
    writeToLog("CHIP-8 Emulator Log");
    writeToLog("Started at: " + timestamp);
    writeToLog("========================================");
    writeToLog("");
}

// Write message to both log file and console
void CHIP8::writeToLog(const string& message) {
    if (log_file.is_open()) {
        log_file << message << endl;
        log_file.flush();  // Flush to ensure data is written immediately
    }
}

// Constructor
CHIP8::CHIP8() : pc(PROGRAM_START), sp(0), delay_timer(0), sound_timer(0), rom_loaded(false) {
    // Initializes memory, registers, keys, display, and opcode to zero
    memset(memory, 0, MEMORY_SIZE);
    memset(V, 0, sizeof(V));        
    memset(key, 0, sizeof(key));    
    memset(display, 0, sizeof(display));
    memset(stack, 0, sizeof(stack));
    
    I = 0;
    opcode = 0;
    
    // Initialize logging
    initializeLogging();
    
    // Load fontset into memory
    for (int i = 0; i < FONTSET_SIZE; ++i) {
        memory[FONTSET_START + i] = chip8_fontset[i];
    }
    
    // Initialize timing
    last_cycle_time = high_resolution_clock::now();
    last_timer_update = high_resolution_clock::now();
}

// Destructor
CHIP8::~CHIP8() {
    writeToLog("");
    writeToLog("========================================");
    writeToLog("Finished running!");
    writeToLog("========================================");
    if (log_file.is_open()) {
        log_file.close();
    }
}

// Load ROM from file
bool CHIP8::loadROM(const char* filename) {
    ifstream ROM(filename, ios::binary);
    if (!ROM.is_open()) {
        writeToLog(string("Error: Could not open ROM file: ") + filename);
        return false;
    }
    
    // Get file size
    ROM.seekg(0, ios::end);
    streampos rom_size = ROM.tellg();
    ROM.seekg(0, ios::beg);
    
    int rom_bytes = static_cast<int>(rom_size);
    const unsigned short MAX_ROM_SIZE = MEMORY_SIZE - PROGRAM_START;
    
    if (rom_bytes == 0) {
        writeToLog("Error: ROM file is empty!");
        ROM.close();
        return false;
    }
    
    if (rom_bytes > MAX_ROM_SIZE) {
        stringstream ss;
        ss << "Error: ROM File Size (" << rom_bytes << " bytes) is too big! Max size: " << MAX_ROM_SIZE << " bytes";
        writeToLog(ss.str());
        ROM.close();
        return false;
    }
    
    // Read ROM into memory
    ROM.read(reinterpret_cast<char*>(&memory[PROGRAM_START]), rom_bytes);
    ROM.close();
    
    rom_loaded = true;
    
    stringstream ss;
    ss << "ROM loaded successfully: " << rom_bytes << " bytes";
    writeToLog(ss.str());
    
    return true;
}

// Function to increase PC
void CHIP8::incPC() {
    pc += 2;
}

// Displays opcodes in terminal and in log file if debug mode is enabled
void CHIP8::logOpcode(uint16_t op) { 
    if (DEBUG_OPCODES) {
        stringstream debugOpcode;
        debugOpcode << "Opcode: 0x" << hex << setfill('0') << setw(4) << op << dec << setfill(' ') << endl;
        cout << debugOpcode.str();
        writeToLog(debugOpcode.str());
    }
}

// Updates delay and sound timers by one, at 60 Hz if above 0
void CHIP8::updateTimers() { 
    auto current_time = high_resolution_clock::now();
    auto elapsed = duration_cast<milliseconds>(current_time - last_timer_update).count();
    
    if (elapsed >= (1000 / TIMER_SPEED)) {
        if (delay_timer > 0) {
            delay_timer--;
        }
        if (sound_timer > 0) {
            sound_timer--;
        }
        last_timer_update = current_time;
    }
}

// For CXNN
int CHIP8::genRandomNum() { 
    return rand() % 256;
}

// Out of bounds checkers
bool CHIP8::isValidMemoryAddress(uint16_t address) { 
    return address < MEMORY_SIZE;
}

bool CHIP8::isValidStackPointer() { 
 return sp < STACK_SIZE;
}

bool CHIP8::isValidKeyIndex(uint8_t key_index) {
    return key_index < 16;
}

// Handle keyboard input
void CHIP8::handleKeyEvent(SDL_KeyboardEvent key_event) {
    uint8_t key_state = (key_event.down) ? 1 : 0;
    
  switch (key_event.scancode) {
    case SDL_SCANCODE_1: key[0x1] = key_state; break;
    case SDL_SCANCODE_2: key[0x2] = key_state; break;
    case SDL_SCANCODE_3: key[0x3] = key_state; break;
    case SDL_SCANCODE_4: key[0xC] = key_state; break;
    case SDL_SCANCODE_Q: key[0x4] = key_state; break;
    case SDL_SCANCODE_W: key[0x5] = key_state; break;
    case SDL_SCANCODE_E: key[0x6] = key_state; break;
    case SDL_SCANCODE_R: key[0xD] = key_state; break;
    case SDL_SCANCODE_A: key[0x7] = key_state; break;
    case SDL_SCANCODE_S: key[0x8] = key_state; break;
    case SDL_SCANCODE_D: key[0x9] = key_state; break;
    case SDL_SCANCODE_F: key[0xE] = key_state; break;
    case SDL_SCANCODE_Z: key[0xA] = key_state; break;
    case SDL_SCANCODE_X: key[0x0] = key_state; break;
    case SDL_SCANCODE_C: key[0xB] = key_state; break;
    case SDL_SCANCODE_V: key[0xF] = key_state; break;
    default: break;
    }
}

// Execute opcode
void CHIP8::execute_opcode() {
    // Fetch
    if (!isValidMemoryAddress(pc) || !isValidMemoryAddress(pc + 1)) {
    cerr << "Error: Program counter out of bounds (0x" << hex << pc << dec << ")" << endl;
        rom_loaded = false;
        return;
    }
    
    opcode = (memory[pc] << 8) | memory[pc + 1];
    logOpcode(opcode);
    
    // Decode and Execute
    if ((opcode >> 12) == 1) { // 1NNN: jump
        pc = opcode & 0x0FFF;
    }
    else if ((opcode >> 12) == 2) { // 2NNN: call
        if (sp >= STACK_SIZE - 1) {
            cerr << "Error: Stack overflow at 0x" << hex << pc << dec << endl;
            rom_loaded = false;
            return;
        }
        sp++;
        stack[sp] = pc;
        pc = opcode & 0x0FFF;
    }
    else if ((opcode >> 12) == 3) { // 3XNN: Skip if V[X] = NN
        if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
            incPC();
            incPC();
        }
        else {
            incPC();
        }
    }
    else if ((opcode >> 12) == 4) { // 4XNN: Skip if V[X] != NN
        if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
            incPC();
            incPC();
        }
        else {
            incPC();
        }
    }
    else if ((opcode >> 12) == 5) { // 5XY0: Skip if V[X] = V[Y]
        if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]) {
            incPC();
            incPC();
        }
        else {
            incPC();
        }
    }
    else if ((opcode >> 12) == 6) { // 6XNN: set register vx
        V[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF);
        incPC();
    }
    else if ((opcode >> 12) == 7) { // 7XNN: add value to register V[X]
        V[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
        incPC();
    }
    else if ((opcode >> 12) == 8) { // 8XXX
        uint16_t tempOpcode = opcode & 0x000F;
        uint8_t VX = (opcode & 0x0F00) >> 8;
        uint8_t VY = (opcode & 0x00F0) >> 4;
        uint16_t sum = V[VX] + V[VY];
        
        switch (tempOpcode) {
            case 0x0: // 8XY0: Set V[X] to V[Y]
                V[VX] = V[VY];
                incPC();
                break;
            case 0x1: // 8XY1: V[X] is set to the bitwise OR of V[X] and V[Y]
                V[VX] |= V[VY];
                incPC();
                break;
            case 0x2: // 8XY2: V[X] is set to the bitwise AND of V[X] and V[Y]
                V[VX] &= V[VY];
                incPC();
                break;
            case 0x3: // 8XY3: V[X] is set to the bitwise XOR of V[X] and V[Y]
                V[VX] ^= V[VY];
                incPC();
                break;
            case 0x4: // 8XY4: V[X] is set to V[X] + V[Y]
                V[0xF] = (sum > 255) ? 1 : 0;
                V[VX] = sum & 0xFF;
                incPC();
                break;
            case 0x5: // 8XY5: V[X] is set to V[X] - V[Y]
                V[0xF] = (V[VX] >= V[VY]) ? 1 : 0;
                V[VX] -= V[VY];
                incPC();
                break;
            case 0x6: // 8XY6: Shift V[X] one bit to the right
                //V[VX] = V[VY]; //classic quirk
                V[0xF] = V[VX] & 0x1;
                V[VX] >>= 1;
                incPC();
                break;
            case 0x7: // 8XY7: V[X] is set to V[Y] - V[X]
                V[0xF] = (V[VY] >= V[VX]) ? 1 : 0;
                V[VX] = V[VY] - V[VX];
                incPC();
                break;
            case 0xE: // 8XYE: Shift V[X] one bit to the left
                //V[VX] = V[VY]; // classic quirk
                V[0xF] = (V[VX] & 0x80) >> 7;
                V[VX] <<= 1;
                incPC();
                break;
            default:
                cout << "Could not find opcode! Nibble: 8 Opcode: 0x" << hex << opcode << dec << endl;
                incPC();
                break;
        }
    }
    else if ((opcode >> 12) == 9) { // 9XY0 Skip if V[X] != V[Y]
        if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4]) {
            incPC();
            incPC();
        }
        else {
            incPC();
        }
    }
    else if ((opcode >> 12) == 0xA) { //ANNN: set index register I
        I = (opcode & 0x0FFF);
        incPC();
    }
    else if ((opcode >> 12) == 0xB) { //BNNN: set pc to NNN + V[0]
        pc = (opcode & 0x0FFF) + V[0];
    }
    else if ((opcode >> 12) == 0xC) { //CXNN: sets V[X] to random number & NN
        V[(opcode & 0x0F00) >> 8] = genRandomNum() & (opcode & 0x00FF);
        incPC();
    }
    else if ((opcode >> 12) == 0xD) { // DXYN: draws to display
        uint8_t xCoord = V[(opcode & 0x0F00) >> 8] % CHIP8_WIDTH; // % to make sure value is useable. For example, if V[X] is 70, 70 % 64 = 6 so the x coord is 6.
        uint8_t yCoord = V[(opcode & 0x00F0) >> 4] % CHIP8_HEIGHT;
        int N = opcode & 0x000F;
        V[0xF] = 0;
        
        for (int i = 0; i < N; ++i) {
            if (!isValidMemoryAddress(I + i)) { // I + i is the memory address where sprite data is loaded
                cerr << "Error: Attempting to read sprite data from invalid memory address (0x" << hex << (I + i) << dec << ")" << endl;
                rom_loaded = false;
                break;
            }
   
            uint8_t spriteByte = memory[I + i];
            uint8_t drawY = (yCoord + i);
            if (drawY >= CHIP8_HEIGHT) {
                break;
            }
            if (xCoord + 8 > CHIP8_WIDTH) {
                uint8_t pixelsToClip = (xCoord + 8) - CHIP8_WIDTH; // One sprite is 8 bits. If xCoord was 62, 62 + 8 = 70 which is larger than 64.  6 bits need to be clipped.
                if (pixelsToClip > 0) {
                    spriteByte &= (0xFF << pixelsToClip); // So, it would clip the 6 rightmost bits.
                }
            }
            int shiftAmount = CHIP8_WIDTH - 8 - xCoord; // The amount to shift so it fits into the 64 bit display array.
            uint64_t spriteVal = ((uint64_t)spriteByte) << shiftAmount;
            if (display[drawY] & spriteVal) { // Checks for collisions
                V[0xF] = 1;
            }
            display[drawY] ^= spriteVal;
        }
        incPC();
    }
    else if ((opcode >> 12) == 0xE) { // EXXX
        uint8_t VX = (opcode & 0x0F00) >> 8;
        uint8_t keyValue = V[VX];
    
        if (!isValidKeyIndex(keyValue)) {
            cerr << "Warning: Key index out of bounds: " << (int)keyValue << endl;
            incPC();
            return;
        }
    
        switch (opcode & 0x00FF) {
            case 0x9E: // EX9E: Skip next instruction if key with the value of Vx is pressed.
                if (key[keyValue]) {
                    incPC();
                    incPC();
                }
                else {
                    incPC();
                }
                break;
            case 0xA1: // EXA1: Skip next instruction if key with the value of Vx is not pressed.
                if (!key[keyValue]) {
                    incPC();
                    incPC();
                }
                else {
                    incPC();
                }
            break;
            default:
                cout << "Could not find opcode! Nibble: E Opcode: 0x" << hex << opcode << dec << endl;
                incPC();
                break;
        }
    }
    else if ((opcode >> 12) == 0xF) { // FXXX
        uint8_t VX = (opcode & 0x0F00) >> 8;
        switch (opcode & 0x00FF) {
            case 0x07: // FX07: Set V[X] to the value of the delay timer.
                V[VX] = delay_timer;
                incPC();
                break;
            case 0x0A: // FX0A: Wait for a key press, store the value of the key in Vx.
            {
                bool key_pressed = false;
                for (int i = 0; i < 16; ++i) {
                    if (key[i]) {
                        V[VX] = i;
                        key_pressed = true;
                        break;
                    }
                }
                if (key_pressed) {
                    incPC();
                }
            }
            break;
            case 0x15: // FX15: Set the delay timer to V[X].
                delay_timer = V[VX];
                incPC();
                break;
            case 0x18: // FX18: Set the sound timer to V[X].
                sound_timer = V[VX];
                incPC();
                break;
            case 0x1E: // FX1E: Add V[X] to I. V[F] is set to 1 if there is a carry, 0 if there isn't.
            {
                uint16_t sum = I + V[VX];
                V[0xF] = (sum > 0xFFF) ? 1 : 0;
                I = sum & 0xFFF;
                incPC();
            }
            break;
            case 0x29: // FX29: Set I to the location of the sprite data for digit V[X].
                if (V[VX] > 0xF) {
                    cerr << "Warning: FX29 - V[X] value (" << (int)V[VX] << ") exceeds valid font digit range (0-15)" << endl;
                }
                I = FONTSET_START + (V[VX] * 5);
                incPC();
                break;
            case 0x33: // FX33: Store the binary-coded decimal representation of V[X] in memory at I, I+1, I+2.
            {
                if (!isValidMemoryAddress(I) || !isValidMemoryAddress(I + 1) || !isValidMemoryAddress(I + 2)) {
                    cerr << "Error: FX33 attempting to write to invalid memory address starting at 0x" << hex << I << dec << endl;
                    rom_loaded = false;
                    break;
                }
            
                uint8_t value = V[VX];
                memory[I] = value / 100;
                memory[I + 1] = (value / 10) % 10;
                memory[I + 2] = value % 10;
                incPC();
            }
            break;
            case 0x55: // FX55: Store V[0] to V[X] in memory starting at location I.
            {
                if (!isValidMemoryAddress(I) || !isValidMemoryAddress(I + VX)) {
                    cerr << "Error: FX55 attempting to write to invalid memory address range starting at 0x" << hex << I << dec << endl;
                    rom_loaded = false;
                    break;
                }
    
                for (int i = 0; i <= VX; ++i) {
                    memory[I + i] = V[i];
                }
                //I += VX + 1; // classic behaviour
                incPC();
            }
        break;
        case 0x65: // FX65: Read V[0] to V[X] from memory starting at location I.
        {
            if (!isValidMemoryAddress(I) || !isValidMemoryAddress(I + VX)) {
                cerr << "Error: FX65 attempting to read from invalid memory address range starting at 0x" << hex << I << dec << endl;
                rom_loaded = false;
                break;
            }
       
            for (int i = 0; i <= VX; ++i) {
                V[i] = memory[I + i];
            }

            //I += VX + 1; // classic behaviour
            incPC();
        }
        break;
        default:
            cout << "Could not find opcode! Nibble: F Opcode: 0x" << hex << opcode << dec << endl;
            incPC();
            break;
        }
    }
    else {
        switch (opcode) {
        case 0x00E0: // 0x00E0 (clear screen)
            for (int i = 0; i < 32; ++i) {
                display[i] = 0;
            }
            incPC();
            break;
        case 0x00EE: // 0x00EE (return from subroutine)
            if (sp == 0) {
                cerr << "Error: Stack underflow on return instruction" << endl;
                rom_loaded = false;
                break;
            }
     
            pc = stack[sp];
            sp--;
            incPC();
            break;
      default:
            cout << "Unknown opcode: 0x" << hex << opcode << dec << endl;
            incPC();
            break;
        }
    }
}

// Main emulation loop
void CHIP8::run() {
    // This part initializes the SDL Window, Renderer and Texture

    if (!rom_loaded) {
        cerr << "Error: No ROM loaded. Cannot start emulation." << endl;
        return;
    }
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        cerr << "SDL_Init failed: " << SDL_GetError() << endl;
        return;
    }
    
    // Create window
    SDL_Window* window = SDL_CreateWindow("CHIP-8 Emulator", 640, 320, SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << endl;
        SDL_Quit();
        return;
    }
    
    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == NULL) {
        cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }
    
    // Create texture
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, CHIP8_WIDTH, CHIP8_HEIGHT);
    if (texture == NULL) {
        cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }
    
    if (!SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST)) {
        cerr << "Could not scale texture! SDL_Error: " << SDL_GetError() << endl;
    }
    
    const uint32_t PIXEL_ON = 0xFFFFFFFF;
    const uint32_t PIXEL_OFF = 0xFF000000;
    
    bool running = true;
    SDL_Event event;
    
    // Gets current time with highest precision available
    // Auto automatically assigns the variable type
    last_cycle_time = high_resolution_clock::now();
    last_timer_update = high_resolution_clock::now();
    auto last_render_time = high_resolution_clock::now();
    
    pc = PROGRAM_START;
    sp = 0;
    
    while (running && rom_loaded) {
      // Handle SDL events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
                handleKeyEvent(event.key);
            }
        }
        
        // Update timers
        updateTimers();
     
        // Fetch, Decode, Execute
        execute_opcode();
        
        // Render screen every ~16.67ms (60 FPS)
        auto current_render_time = high_resolution_clock::now();
        auto render_elapsed = duration_cast<milliseconds>(current_render_time - last_render_time).count();
        
        // Main display updater. Locks texture, applies changes, then unlocks texture and updates screen
        if (render_elapsed >= (1000 / 60)) {
            // Lock texture
            void* pixels_ptr = nullptr;
            int pitch = 0;
            int lockResult = SDL_LockTexture(texture, NULL, &pixels_ptr, &pitch); // Locks entire screen and returns updated pitch and pixels_ptr
            
            if (lockResult != 0) {
                uint32_t* texture_pixels = static_cast<uint32_t*>(pixels_ptr);
       
                // Render display
                for (int y = 0; y < 32; y++) {
                    for (int x = 0; x < 64; x++) {
                        // Gets each line of the updated display and checks each bit if its on or off
                        uint64_t line = display[y];
                        int bit_position = 63 - x;
                        uint64_t bit = (line >> bit_position) & 1;
      
                        int pixel_offset = y * (pitch / sizeof(uint32_t)) + x;
                        texture_pixels[pixel_offset] = bit ? PIXEL_ON : PIXEL_OFF;
                    }
                }
            }
   
            SDL_UnlockTexture(texture);
     
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
     
            last_render_time = current_render_time;

            
        }
      
        // Maintain clock speed by only executing one cpu instruction every 1 ms
        auto current_time = high_resolution_clock::now();
        auto elapsed = duration_cast<microseconds>(current_time - last_cycle_time).count();
        int desired_cycle_time_us = (500000 / CLOCK_SPEED); // Should be 1000000 but 500000 felt better for me. Adjust as needed
  
        if (elapsed < desired_cycle_time_us) {
            SDL_Delay((desired_cycle_time_us - elapsed) / 1000); // Delays to ensure each instruction is executed every 1 ms.
        }
        last_cycle_time = high_resolution_clock::now();
    }
    
    // Clean up SDL resources
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
