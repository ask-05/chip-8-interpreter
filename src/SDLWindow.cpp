#include <SDL3/SDL.h>
#include <string.h> // for memcpy

// --- Constants (adjust as needed) ---
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define TOTAL_PIXELS (SCREEN_WIDTH * SCREEN_HEIGHT)

// --- Global variables (or pass as arguments) ---
Uint8 display_array[TOTAL_PIXELS]; // Holds 0 (off) or 1 (on)
SDL_Texture* screen_texture = NULL;
SDL_Renderer* renderer = NULL;

// --- Colors (ARGB format for SDL_PIXELFORMAT_RGBA8888) ---
// Note: Actual byte order in memory might be different depending on endianness.
// Here we use AARRGGBB notation conceptually.
#define COLOR_ON  0xFFFFFFFF // White (full alpha, full RGB)
#define COLOR_OFF 0xFF000000 // Black (full alpha, zero RGB)

/**
 * @brief Initializes the texture once.
 */
int initialize_texture(SDL_Renderer* r, int w, int h) {
    renderer = r;
    // Create a streaming texture for frequent updates
    screen_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888, // A common 32-bit format (Alpha, Red, Green, Blue)
        SDL_TEXTUREACCESS_STREAMING,
        w, h
    );
    if (!screen_texture) {
        SDL_Log("SDL_CreateTexture failed: %s", SDL_GetError());
        return -1;
    }
    return 0;
}


/**
 * @brief Updates the entire screen from the global display_array.
 */
void update_screen_from_array() {
    if (!renderer || !screen_texture) {
        return; // Texture/Renderer not initialized
    }

    void* texture_pixels = NULL;
    int pitch;

    // 1. Lock the texture for direct pixel access
    if (SDL_LockTexture(screen_texture, NULL, &texture_pixels, &pitch) != 0) {
        SDL_Log("SDL_LockTexture failed: %s", SDL_GetError());
        return;
    }

    // `pitch` is the length of one row in bytes.
    // The texture pixel data is now in `texture_pixels`.
    Uint32* pixels = (Uint32*)texture_pixels;

    // 2. Map the 0/1 array to 32-bit color pixels
    for (int i = 0; i < TOTAL_PIXELS; ++i) {
        // Since pitch is the number of bytes per row, and our pixel format is 4 bytes (Uint32)
        // the index calculation simplifies to:
        // row * (pitch / sizeof(Uint32)) + col
        // For a single flat array:
        pixels[i] = (display_array[i] == 1) ? COLOR_ON : COLOR_OFF;
    }

    // 3. Unlock the texture to notify SDL the data is ready
    SDL_UnlockTexture(screen_texture);

    // 4. Render the texture to the full screen
    SDL_RenderClear(renderer); // Optional: Clear the backbuffer first
    SDL_RenderTexture(renderer, screen_texture, NULL, NULL); // NULL for source/dest rects uses entire texture/screen

    // 5. Present the backbuffer to the screen
    SDL_RenderPresent(renderer);
}