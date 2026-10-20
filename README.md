# Wave Effect

A water mandala animation written in C++17 with SDL2, ported from the original
Pygame implementation in `main.py`.

The 600 × 600 window displays translucent, rotating curves with fading trails.
The original motion equations, colors, density ranges and 60 FPS target are
preserved. Random patterns and pixel-level line rendering can differ between
the C++ and Python versions.

## Build and run

Requirements: a C++17 compiler, CMake 3.16 or newer, and SDL2 development files
(SDL3 is not a substitute).

On Debian/Ubuntu:

```sh
sudo apt install build-essential cmake libsdl2-dev
```

From the repository directory:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/wave-effect
```

- Click any mouse button to generate a new pattern.
- Press F11 to toggle fullscreen; press it again to restore the window.
- Press Escape or close the window to quit.

Fullscreen uses the current display's desktop resolution. Both render textures
are recreated at the actual renderer output size in pixels (including HiDPI),
and the curve geometry and center adapt to that size. The 600 × 600 image is
not upscaled. A size change starts a fresh pattern and clears the old trails.
The output size is checked each frame to handle monitor/DPI changes as well.
See SDL2's [renderer output size documentation](https://wiki.libsdl.org/SDL2/SDL_GetRendererOutputSize).

The C++ version does not need Python or Pygame. The original Python files remain
available for comparison; run that version with `uv run main.py`.

## Check without a display

```sh
ctest --test-dir build --output-on-failure
SDL_VIDEODRIVER=dummy ./build/wave-effect --frames 120
```

`--frames N` exits after a positive number of frames. The CTest smoke test uses
SDL's dummy video driver and software renderer to check initialization, drawing,
and shutdown. Interactive runs use accelerated rendering when available and
fall back to software rendering.

Drawing uses separate trail and transparent line textures. Lines replace pixels
within their layer, then that layer is alpha blended over the trail, matching
the original Pygame compositing order. See the SDL2 documentation for
[render targets](https://wiki.libsdl.org/SDL2/SDL_SetRenderTarget) and
[drawing blend modes](https://wiki.libsdl.org/SDL2/SDL_SetRenderDrawBlendMode).
