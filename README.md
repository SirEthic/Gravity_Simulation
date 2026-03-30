# 

# 

# ```

# \# 🌌 3D Solar System Gravity Simulation

# 

# A real-time N-body gravity simulation of our Solar System rendered with \*\*C++\*\* and \*\*OpenGL\*\*.

# All planetary motion is driven entirely by gravitational physics — no hardcoded orbits, no keyframe animations.

# Every body attracts every other body according to Newtonian gravity.

# 

# \---

# 

# \## ✨ Features

# 

# \### Physics

# \- \*\*N-body gravitational simulation\*\* — every body attracts every other body in real time

# \- \*\*Symplectic Euler integration\*\* with configurable timestep and adaptive sub-stepping

# \- \*\*Softened gravity\*\* to prevent numerical singularities at close encounters

# \- \*\*Orbital inclinations\*\* for realistic 3D planetary trajectories

# 

# \### Bodies

# \- ☀️ \*\*Sun\*\* with emissive procedural corona glow

# \- 🪐 \*\*8 planets\*\* — Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus, Neptune

# \- 🌙 \*\*Moon\*\* orbiting Earth with proper relative velocity

# \- 💫 \*\*Pluto\*\* with a steep 17° orbital inclination

# \- ☄️ \*\*Halley's Comet\*\* on a highly eccentric inclined orbit

# \- 🪨 \*\*180 procedurally generated asteroids\*\* forming a belt between Mars and Jupiter

# 

# \### Rendering

# \- \*\*Point-sprite sphere rendering\*\* with per-pixel Phong shading

# \- \*\*Day/night terminators\*\* on planet surfaces

# \- \*\*Atmospheric rim lighting\*\* for a subtle glow effect

# \- \*\*Saturn's rings\*\* rendered as translucent multi-band line geometry

# \- \*\*Orbital trails\*\* with smooth fade-out over time

# \- \*\*2500 background stars\*\* for a deep-space skybox feel

# \- \*\*Reference grid\*\* on the orbital plane

# \- \*\*4× MSAA\*\* for smooth edges

# 

# \### Interactivity

# \- Orbit and zoom camera with mouse

# \- Pause/resume simulation

# \- Follow individual planets

# \- Toggle orbital trails on/off

# \- Full reset at any time

# 

# \---

# 

# \## 🎮 Controls

# 

# | Input | Action |

# |---|---|

# | \*\*Left Drag\*\* | Orbit camera around target |

# | \*\*Scroll Wheel\*\* | Zoom in / out |

# | \*\*Space\*\* | Pause / Resume simulation |

# | \*\*R\*\* | Reset simulation to initial state |

# | \*\*T\*\* | Toggle orbital trails |

# | \*\*F\*\* | Cycle follow target through planets |

# | \*\*ESC\*\* | Quit |

# 

# \---

# 

# \## 🛠️ Build Instructions

# 

# \### Requirements

# 

# \- \*\*C++11\*\* or later

# \- \*\*OpenGL 3.3+\*\*

# \- \[GLFW](https://www.glfw.org/) — windowing and input

# \- \[GLEW](http://glew.sourceforge.net/) — OpenGL extension loading

# \- \[GLM](https://github.com/g-truc/glm) — math library (header-only)

# 

# \### Linux

# 

# ```bash

# \# Install dependencies (Debian/Ubuntu)

# sudo apt install libglfw3-dev libglew-dev libglm-dev

# 

# \# Build

# g++ -std=c++11 -O2 -o solar\_system main.cpp -lGL -lGLEW -lglfw -lm

# 

# \# Run

# ./solar\_system

# ```

# 

# \### Windows (MinGW)

# 

# ```bash

# g++ -std=c++11 -O2 -o solar\_system.exe main.cpp -lglew32 -lglfw3 -lopengl32 -lgdi32

# solar\_system.exe

# ```

# 

# \### Windows (MSVC / Visual Studio)

# 

# 1\. Create a new empty C++ project

# 2\. Add `main.cpp` to the project

# 3\. Link against `glew32.lib`, `glfw3.lib`, and `opengl32.lib`

# 4\. Ensure GLFW, GLEW, and GLM headers are in your include path

# 5\. Build and run

# 

# \### macOS

# 

# ```bash

# \# Install dependencies (Homebrew)

# brew install glfw glew glm

# 

# \# Build

# g++ -std=c++11 -O2 -o solar\_system main.cpp -lGLEW -lglfw -framework OpenGL

# 

# \# Run

# ./solar\_system

# ```

# 

# \---

# 

# \## ⚙️ How It Works

# 

# \### Gravity

# 

# The simulation computes gravitational force between every pair of bodies using:

# 

# ```

# F = G × m₁ × m₂ / (r² + ε²)

# ```

# 

# Where:

# \- `G` is the gravitational constant (scaled for the simulation)

# \- `r` is the distance between two bodies

# \- `ε` is a softening parameter that prevents numerical blowup at very small distances

# 

# \### Integration

# 

# Each simulation step:

# 

# 1\. \*\*Compute accelerations\*\* — loop over all unique pairs, accumulate gravitational forces

# 2\. \*\*Update velocities\*\* — `v += a × dt`

# 3\. \*\*Update positions\*\* — `p += v × dt`

# 4\. \*\*Record trails\*\* — store position history for rendering orbital paths

# 

# The main loop uses \*\*adaptive sub-stepping\*\* based on real frame time to keep physics stable regardless of framerate.

# 

# \### Initialization

# 

# Planets are placed at their approximate relative distances from the Sun and given initial velocities calculated for \*\*circular or inclined orbits\*\*:

# 

# ```

# v\_circular = sqrt(G × M\_sun / r)

# ```

# 

# The velocity vector is perpendicular to the radius in the orbital plane, then rotated by the inclination angle. This ensures planets begin in stable orbits from the first frame.

# 

# \### Rendering Pipeline

# 

# Each body is rendered as a \*\*GL\_POINTS\*\* point sprite. In the fragment shader, the point is turned into a 3D sphere by:

# 

# 1\. Computing a surface normal from `gl\\\_PointCoord`

# 2\. Applying \*\*Phong shading\*\* with the Sun as the light source

# 3\. Adding \*\*specular highlights\*\*, \*\*rim lighting\*\*, and \*\*day/night terminator\*\* effects

# 4\. The Sun uses a separate emissive path with a procedural \*\*corona glow\*\*

# 

# \---

# 

# The entire project is contained in a \*\*single self-contained C++ file\*\* with inline GLSL shaders. No external assets or textures are needed.

# 

# \---

# 

# \## 📊 Simulation Parameters

# 

# | Parameter | Value | Description |

# |---|---|---|

# | `G` | 0.0005 | Gravitational constant (simulation scale) |

# | `DT` | 0.0015 | Physics timestep |

# | `SOFTENING` | 0.08 | Gravity softening to prevent singularities |

# | `TRAIL\\\_LEN` | 180 | Max trail points per body |

# | `NUM\\\_ASTEROIDS` | 180 | Number of asteroid belt bodies |

# | `Sun mass` | 1000 | Dominant central mass |

# 

# \---

# 

# \## 🌍 Planet Data

# 

# | Body | Distance | Mass | Radius | Inclination |

# |---|---|---|---|---|

# | Sun | 0.0 | 1000.0 | 1.40 | — |

# | Mercury | 3.6 | 0.40 | 0.10 | 7.0° |

# | Venus | 5.8 | 0.90 | 0.20 | 3.4° |

# | Earth | 8.2 | 1.00 | 0.22 | 0.0° |

# | Moon | 8.9 | 0.06 | 0.07 | 0.0° |

# | Mars | 12.0 | 0.65 | 0.15 | 1.9° |

# | Jupiter | 22.0 | 12.0 | 0.66 | 1.3° |

# | Saturn | 32.0 | 10.0 | 0.56 | 2.5° |

# | Uranus | 43.0 | 5.0 | 0.42 | 0.8° |

# | Neptune | 54.0 | 5.0 | 0.40 | 1.8° |

# | Pluto | 72.0 | 0.20 | 0.07 | 17.0° |

# | Halley | 5.0 | 0.05 | 0.08 | Eccentric |

# 

# \*Values are in simulation units, proportionally inspired by real Solar System ratios.\*

# 

# \---

# 

# \## 📸 Screenshots

# 

# ```

# !\[Overview](screenshots/overview.png)

# !\[Saturn Close-up](screenshots/saturn.png)

# !\[Earth and Moon](screenshots/earth\_moon.png)

# ```

# 

# \---

# 

# \## 📝 License

# 

# MIT License

# 

# Copyright (c) 2024

# 

# Permission is hereby granted, free of charge, to any person obtaining a copy

# of this software and associated documentation files (the "Software"), to deal

# in the Software without restriction, including without limitation the rights

# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell

# copies of the Software, and to permit persons to whom the Software is

# furnished to do so, subject to the following conditions:

# 

# The above copyright notice and this permission notice shall be included in all

# copies or substantial portions of the Software.

# 

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR

# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,

# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE

# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER

# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,

# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE

# SOFTWARE.

# ```

