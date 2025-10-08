#define SDL_MAIN_USE_CALLBACKS 1

#include <iostream>
#include <fstream>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>



using namespace std;

// VARIABLE DECLARATION
uint16_t opcode; // opcode = operational code. codes that when decoded, tell us a specific task (confirm))
uint8_t memory[4096]; // this is basically the ram. 4K ram.
uint8_t V[16]; // variable registers V0-VF. VF is a flag register.
uint16_t I; // index register. points at locations in memory.
uint16_t pc; // program counter. points to current instruction in memory
uint16_t stack[16]; // stack. stores PC addresses to return from subroutines
uint8_t sp; // stack pointer
uint8_t delay_timer; // decremnted at 60hz until it reaches 0.
uint8_t sound_timer; // same as delay_timer but it gives off a beeping sound as long as it's not 0.

uint32_t display[64 * 32]; // SDL display

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


SDL_Window* window = NULL;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		return SDL_APP_FAILURE;
	}
	for (int i = 0; i < 80; ++i) {
		memory[0x50 + i] = chip8_fontset[i];
	}

	window = SDL_CreateWindow("CHIP-8 Emulator", 640, 320, SDL_WINDOW_RESIZABLE);

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event *event) {
	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}
	else if (event->type == SDL_EVENT_KEY_DOWN) {
		if (event->key.key >= 48 && event->key.key <= 57) {
			cout << "KEY NUMBER: " << event->key.key << endl;
			for (int i = 0; i <= 4; i++) {
				cout << to_binary_string(memory[(0x50 + i) + (5*(event->key.key - 48))]) << endl;	
			}
			cout << "\n-------------\n";
		}
	}
	else if (event->type == SDL_EVENT_FINGER_DOWN) {
		SDL_Log("x = %f, y = %f", event->tfinger.x, event->tfinger.y);
	}
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	SDL_Quit();
}

