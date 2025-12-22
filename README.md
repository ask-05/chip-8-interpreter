# CHIP-8 Emulator

This is a CHIP-8 Emulator created in SDL3 and C++. This is based on the interpreted programming language developed in the 1970s by Joseph Weisbecker.

## Building and Compling Instructions
### Linux
Install the C++ compilers along with SDL3. These commands will depend on your distribution.

`sudo apt install build-essential libsdl3-dev` 
`sudo pacman -S base-devel sdl3`
`sudo dnf install gcc gcc-c++ SDL3-devel`

Next, navigate to .\src and place the SDL3.dll file in there and then run:

`g++ main.cpp file.cpp -o my_game -I/usr/local/include -L/usr/local/lib -lSDL3 -Wl,-rpath,/usr/local/lib
`
` ./chip8-emulator`
### Windows
Install MSYS2 and run the MSYS2 MinGW64 terminal and install MinGW compiler along with SDL3 libraries.

`pacman -Syu mingw-w64-ucrt-x86_64-toolchain`
`pacman -S mingw-w64-ucrt-x86_64-sdl3`

**Note: This is for 64 bit computers.**

Next, navigate to .\src and place the SDL3.dll file in there and then run:

`g++ main.cpp chip8.cpp -o chip8-emulator.exe -I "<location to SDL include folder>" -L "<location to SDL lib/x64 folder>" -lSDL3`

The .exe file should be in the same directory ready for you to open.
## CHIP-8 Structure
#### CHIP-8 Components

|  Component | Description  |
| ------------ | ------------ |
| Memory  | Has access to up to 4 KB of RAM |
| Display  | 64 x 32 pixels monochrome  |
| Program Counter  | Points at current instruction in memory  |
| Index Register  | Points at instructions in memory  |
| Stack  | Calls subroutines and returns from them  |
| Delay Timer  | Decremented at a rate of 60 Hz  |
| Sound Timer  | Decremented at a rate of 60 Hz and beeps when not 0  |
| Variable Registers  | V[0] to V[F] for general purpose. VF is a flag register which is set to 0 or 1 depending on some rule on an opcode.  |

#### Opcodes

| Opcode  | Description  |
| ------------ | ------------ |
| 0NNN | Pauses execution and calls subroutine at NNN. **Only to be implemented for the COSMAC VIP or ETI-660. Not implemented in this program.**  |
| 00E0 | Clears display  |
| 00EE | Pops last address from stack and sets PC to it.  |
| 1NNN | Sets PC to NNN. Does not increment PC.  |
| 2NNN | Pushes current PC to stack and then sets PC to NNN.  |
| 3XNN  | Skips an instruction if V[X] == NN  |
| 4XNN  | Skips an instruction if V[X] **!=** NN  |
| 5XY0  | Skips an instruction if V[X] == V[Y]  |
| 6XNN  | Sets register V[X] == NN  |
| 7XNN  | Adds NN to V[X]  |
| 8XY0  | Sets V[X] = V[Y]  |
| 8XY1  | Sets V[X] to the OR of V[X] and V[Y]  |
| 8XY2  | Sets V[X] to the AND of V[X] and V[Y]  |
| 8XY3  | Sets V[X] to the XOR of V[X] and V[Y]  |
| 8XY4  | Sets V[X] to V[X] + V[Y]  |
| 8XY5  | Sets V[X] to V[X] - V[Y]  |
| 8XY6  | Shift V[X] one bit to the right. Sets V[F] to 1 if the bit shifted out was 1.  |
| 8XY7  | Sets V[X] to V[Y] - V[X]  |
| 8XYE  | Shift V[X] one bit to the left. Sets V[F] to 1 if the bit shifted out was 1.  |
| 9XY0  | Skips an instruction if V[X] **!=** V[Y]  |
| ANNN  | Sets Index Register to NNN  |
| BNNN  | Sets PC to NNN + V[0]  |
| CXNN  | V[X] is set to a random number AND NN  |
| DXYN  | Draws an N pixels tall sprite at the coordinates in V[X] and V[Y]. The sprite data is stored in the memory location that the Index Register is holding. If any pixels on the screen turn off, V[F] is set to 1.  |
| EX9E  | Skips an instruction if a key that is pressed matches the value in V[X].  |
| EXA1  | Skips an instruction if a key that is pressed does not match the value in V[X].  |
| FX07  | Sets V[X] to the current value of the delay timer.  |
| FX15  | Sets the delay timer to the value in V[X]  |
| FX18  | Sets the sound timer to the value in V[X]  |
| FX1E  | The Index Register is set to I + V[X]  |
| FX0A  | Stops execution and waits for user input. Sets V[X] to the key pressed.  |
| FX29  | Index Register is set to the location of the sprite data for V[X].  |
| FX33  | Stores the BCD representation of V[X] in memory locations I, I + 1, and I + 2.  |
| FX55  | Stores the registers (V[0] to V[F] in memory starting at location I).  |
| FX65  | Reads the registers (V[0] to V[F] in memory starting at location I).  |

#### Clocks
| CPU Clock | Timer Clock | Display Clock |
| ------------ | ------------ | ------------ |
| Executes one CPU Instruction every millisecond and makes sure to delay the program if it tries to execute an instruction before 1 millisecond | Decrements the delay and sound timers 60 times a second | Updates the display at 60 FPS or 60 times a second. |

## TODO
- Make a CMAKE file to to automate build and compile process
- Add button to restart the emulator
- Add UI to set ROM folder and choose ROM to load
- Add buttons to adjust clock speed

## Resources

These resources were very helpful:

* https://tobiasvl.github.io/blog/write-a-chip-8-emulator/
* https://wiki.libsdl.org/SDL3/FrontPage
* http://en.wikipedia.org/wiki/CHIP-8
