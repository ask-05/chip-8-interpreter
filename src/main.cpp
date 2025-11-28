// #define SDL_MAIN_USE_CALLBACKS 1

#include <iostream>
#include <fstream>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <bitset>


using namespace std;

// VARIABLE DECLARATION
uint16_t opcode; // opcode = operational code. codes that when decoded, tell us a specific task (confirm)
uint8_t memory[4096]; // this is basically the ram.
uint8_t V[16]; // variable registers V0-VF. VF is a flag register.
uint16_t I; // index register. points at locations in memory.
uint16_t pc; // program counter. points to current instruction in memory
uint16_t stack[16]; // stack. stores PC addresses to return from subroutines
uint8_t sp; // stack pointer
uint8_t delay_timer; // decremented at 60hz until it reaches 0.
uint8_t sound_timer; // same as delay_timer but it gives off a beeping sound as long as it's not 0.

uint64_t display[32]; // SDL display. 32 lines

constexpr int CHIP8_WIDTH = 64;
constexpr int CHIP8_HEIGHT = 32;

//STORING AND LOADING FONT
uint8_t chip8_fontset[80] = {
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
	0xF0, 0x80, 0xF0, 0x80, 0x80, // F
};


// prints out 8 bit binary
string to_binary_string(unsigned char val) {
	string binaryString = "";

	for (int i = 7; i >= 0; i--) {
		if ((val >> i) & 1) {
			binaryString += '1';
		}
		else {
			binaryString += '0';
		}
	}

	return binaryString;
}

// Struct for SDL makes it easy to use
struct SDLWindow {
	SDL_Window* window = NULL;
	SDL_Renderer* renderer = NULL;
	SDL_Texture* texture = NULL;
	SDL_Event event;

	const uint32_t PIXEL_ON = 0xFFFFFFFF;
	const uint32_t PIXEL_OFF = 0xFF000000;

	bool running = true;

	SDLWindow() {
		if (SDL_Init(SDL_INIT_VIDEO) < 0) {
			cerr << "SDL_Init failed: " << SDL_GetError() << endl;
			running = false;
			return;
		}

		window = SDL_CreateWindow("CHIP-8 Emulator", 640, 320, SDL_WINDOW_RESIZABLE);
		if (window == NULL) {
			cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << endl;
			running = false;
			return;
		}

		renderer = SDL_CreateRenderer(window, NULL);
		if (renderer == NULL) {
			cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << endl;
			running = false;
			return;
		}

		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, CHIP8_WIDTH, CHIP8_HEIGHT);
		if (texture == NULL) {
			cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << endl;
			running = false;
			return;
		}

		if (!SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST)) {
			cerr << "Could not scale texture! SDL_Error: " << SDL_GetError() << endl;
		}
		

	}

	~SDLWindow() {
		SDL_Quit();
	}

	// Renders by locking the texture (the entire screen), changing whatever pixels need to be changed, then unlocks the texture
	void renderScreen() {
		if (!texture || !renderer) {
			cerr << "renderScreen aborted: texture=" << texture << " renderer=" << renderer << endl;
			return;
		}

		void* pixels_ptr = nullptr; // When returned, it points to the pixels that need to be locked
		int pitch = 0; // When returned, it holds the length of one row in bytes
		int lockResult = SDL_LockTexture(texture, NULL, &pixels_ptr, &pitch); // Returns the correct pitch and pixels_ptr
		if (lockResult == 0) { 
			cerr << "SDL_LockTexture failed. SDL_GetError(): '" << SDL_GetError() << "'" << endl;
			return;
		}

		uint32_t* texture_pixels = static_cast<uint32_t*>(pixels_ptr);

		// Each display[y] contains one line of 32 pixels
		for (int y = 0; y < 32; y++) {
			for (int x = 0; x < 64; x++) {
				// For each x position, find which bit in display[y] corresponds to it
				uint64_t line = display[y];          
				int bit_position = 63 - x;    
				uint64_t bit = (line >> bit_position) & 1;  // Extract the bit we want

				// Calculate pixel position in texture using pitch
				int pixel_offset = y * (pitch / sizeof(uint32_t)) + x;
				texture_pixels[pixel_offset] = bit ? PIXEL_ON : PIXEL_OFF;
				
				
			}
		}

		SDL_UnlockTexture(texture);

		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderClear(renderer);
		SDL_RenderTexture(renderer, texture, nullptr, nullptr);
		SDL_RenderPresent(renderer); // Applies the changes done
	}

	void incPC() {
		pc += 2;
	}

	// Main SDL Loop
	void SDLLoop() {
		// Setting initial value
		pc = 0x200;
 		while (running) {
			SDL_PollEvent(&event);
				// Fetch Decode Execute Cycle
				opcode = (memory[pc] << 8) | memory[pc + 1]; // Fetches next opcode from memory

			if ((opcode >> 12) == 1) { // 1NNN (jump)
				pc = opcode & 0x0FFF;
			}
			else if ((opcode >> 12) == 2) { // 2NNN (call)
				sp++;
				stack[sp] = opcode + 2; // Stores address of next opcode
				pc = opcode & 0x0FFF;
			}
			else if ((opcode >> 12) == 3) { // 3XNN Skip if V[X] = NN
				if ((opcode & 0x00FF) == V[opcode & 0x0F00 >> 8]) {
					incPC();
					incPC();
				}
				else {
					incPC();
				}
			}
			else if ((opcode >> 12) == 4) { // 4XNN Skip if V[X] != NN
				if ((opcode & 0x00FF) != V[opcode & 0x0F00 >> 8]) {
					incPC();
					incPC();
				}
				else {
					incPC();
				}
			}
			else if ((opcode >> 12) == 5) { // 5XY0 Skip if V[X] = V[Y]
				if ((opcode & 0x0F00 >> 8) == V[opcode & 0x00F0 >> 4]) {
					incPC();
					incPC();
				}
				else {
					incPC();
				}
			}
			else if ((opcode >> 12) == 6) { // 6XNN (set register vx)
				V[(opcode & 0x0F00) >> 8] = (uint8_t)opcode;
				incPC();
			}
			else if ((opcode >> 12) == 7) { // 7NNN (add value to register vx)
				V[(opcode >> 8) ^ 0x0070] += (uint8_t)opcode;
				incPC();
			}
			else if ((opcode >> 12) == 9) { // 9XY0 Skip if V[X] != V[Y]
				if ((opcode & 0x0F00 >> 8) != V[opcode & 0x00F0 >> 4]) {
					incPC();
					incPC();
				}
				else {
					incPC();
				}
			}
			else if ((opcode >> 12) == 0xA) { //ANNN (set index register I)
				I = (opcode & 0x0FFF);
				incPC();
			}
			else if ((opcode >> 12) == 0xD) { // DXYN (draws to display)
				uint8_t xCoord = V[(opcode & 0x0F00) >> 8] % CHIP8_WIDTH;
				uint8_t yCoord = V[(opcode & 0x00F0) >> 4] % CHIP8_HEIGHT;
				int N = opcode & 0x000F; // Height of Sprite
				V[0xF] = 0; // VF flag set to 0

				for (int i = 0; i < N; ++i) {
					uint8_t spriteByte = memory[I + i];
					uint8_t drawY = (yCoord + i);

					if (drawY >= CHIP8_HEIGHT) {
						cout << "broken!" << endl;
						break;
					}

					if (xCoord + 8 > CHIP8_WIDTH) {
						cout << "clipping!" << endl;
						uint8_t pixelsToClip = (xCoord + 8) - CHIP8_WIDTH;
						if (pixelsToClip > 0) {
							spriteByte &= (0xFF << pixelsToClip); // Clears bits that go over screen boundary
						}
					}

					int shiftAmount = CHIP8_WIDTH - 8 - xCoord; // Shifts 8 bit sprite into the display row
					uint64_t spriteVal = ((uint64_t)spriteByte) << shiftAmount;
					if (display[drawY] & spriteVal) {
						V[0xF] = 1;
					}
					display[drawY] ^= spriteVal;



				}

				renderScreen();
				incPC();
			}
			else {
				switch (opcode) {
					case 0x00E0: // 0x00E0 (clear screen)
						for (int i = 0; i < 32; ++i) {
							display[i] = 0;
						}
						renderScreen();
						incPC();
						break;
					case 0x00EE:
						pc = stack[sp];
						sp--;
						break;
					default:
						cout << "opcode " << opcode << " not found!" << endl;

				}
			}


			if (event.type == SDL_EVENT_QUIT) {
				running = false;
			}
			else if (event.type == SDL_EVENT_KEY_DOWN) {
				if (event.key.key >= 49 && event.key.key <= 57) {
					cout << "KEY NUMBER: " << event.key.key << endl;
					for (int i = 0; i <= 4; i++) { // This loop draws 0-9 on the screen
						display[i] = (uint64_t)memory[(0x50 + i) + (5 * (event.key.key - 48))] << 56;
					}
					renderScreen();
					cout << "\n-------------\n";
				}
				else if (event.key.key == 48) {
					opcode = 0x1ABC;
				}
			}

		}
	}
};

int main(int argc, char* argv[]) {

	//Loading fontset to memory
	for (int i = 0; i < 80; ++i) {
		memory[0x50 + i] = chip8_fontset[i];
	}

	const unsigned short MAX_ROM_SIZE = 4096 - 512;


	// Loading ROM
	ifstream ROM("./assets/roms/ibm.ch8", ios::binary);
	if (!ROM.is_open()) {
		cerr << "Error opening the file!" << endl;
		return 1;
	}

	// Checking ROM Size
	ROM.seekg(0, ios::end);
	streampos rom_size = ROM.tellg();
	ROM.seekg(0, ios::beg);

	int rom_bytes = static_cast<int>(rom_size);

	if (rom_size > MAX_ROM_SIZE) {
		cerr << "ROM File Size (" << rom_bytes << " bytes) is too big!";
			return 2;
	}

	ROM.read(reinterpret_cast<char*>(&memory[0x200]), rom_bytes);

	ROM.close();

	// Loading SDL Window. All Control then goes to SDLLoop
	SDLWindow SDLWin;
	SDLWin.SDLLoop();

	return 0;
}