# 3D Gravity Simulation

A real-time N-body gravitational simulation rendered with OpenGL 3.3.  
Particles attract each other via Newton's law of gravitation with a
softening term to prevent singularities.

---

## Features

| Feature | Detail |
|---|---|
| **N-body physics** | Direct O(n²) summation with softened gravity |
| **Leapfrog integrator** | Symplectic, energy-conserving |
| **Two presets** | Random cluster  &  Spiral galaxy |
| **Particle trails** | Ring-buffer trail renderer |
| **Speed-based colouring** | Blue (slow) → Cyan → Yellow → White (fast) |
| **Orbit camera** | Drag + scroll |
| **Live controls** | Pause, reset, add/remove bodies at runtime |

---

## Controls

| Key / Mouse | Action |
|---|---|
| **Left Drag** | Orbit camera |
| **Scroll Wheel** | Zoom in / out |
| **Space** | Pause / Resume |
| **R** | Reset simulation |
| **G** | Toggle Galaxy / Random preset |
| **T** | Toggle particle trails |
| **+ / =** | Add 50 particles |
| **-** | Remove 50 particles |
| **ESC** | Quit |

---

## Dependencies

| Library | Purpose |
|---|---|
| [GLFW 3](https://www.glfw.org/) | Window & input |
| [GLEW](http://glew.sourceforge.net/) | OpenGL extension loading |
| [GLM](https://github.com/g-truc/glm) | Math (vectors, matrices) |
| CMake ≥ 3.14 | Build system |

---

## Building

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install -y \
    build-essential cmake \
    libglfw3-dev libglew-dev libglm-dev \
    libgl-dev
```

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
./gravity_sim
```

### macOS (Homebrew)

```bash
brew install cmake glfw glew glm
mkdir build && cd build
cmake ..
cmake --build .
./gravity_sim
```

### Windows (vcpkg + Visual Studio / Ninja)

```powershell
vcpkg install glfw3 glew glm
mkdir build; cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
.\Release\gravity_sim.exe
```

---

## Launch flags

```bash
./gravity_sim --galaxy   # start in galaxy preset immediately
```

---

## Tweaking the simulation

Open `gravity_sim.cpp` and adjust the constants near the top:

```cpp
static const int   NUM_PARTICLES = 600;    // initial body count
static const float G             = 0.0005f; // gravitational constant
static const float SOFTENING     = 0.15f;  // prevents singularity
static const float DT            = 0.004f; // time-step (smaller = more accurate)
static const int   TRAIL_LENGTH  = 60;     // trail history per particle
```

Increasing `NUM_PARTICLES` beyond ~1000 will slow down on CPU because the
algorithm is O(n²).  For larger counts, consider implementing a Barnes-Hut
tree (O(n log n)) — a great next step!

---

## Architecture

```
main()
 ├─ GLFW window + callbacks
 ├─ GravitySim::step()          ← physics (CPU, direct summation)
 ├─ Grid draw                   ← reference plane
 ├─ Trail draw (GL_LINES)       ← position history
 └─ Particle draw (GL_POINTS)   ← point sprites with custom shaders
```

All rendering uses modern OpenGL (VAOs, VBOs, GLSL 330 shaders).
