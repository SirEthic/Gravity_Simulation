# 

# ```markdown

# \# 🌌 3D Solar System Gravity Simulation

# 

# A real-time N-body gravity simulation of our Solar System rendered in OpenGL.

# All planetary motion is driven by gravitational physics — no hardcoded orbits.

# 

# !\[Solar System](screenshot.png)

# 

# \## Features

# 

# \- \*\*N-body gravitational simulation\*\* — every body attracts every other body

# \- \*\*Sun + 8 planets + Pluto + Moon + Halley's Comet\*\*

# \- \*\*180 asteroids\*\* forming a procedurally generated asteroid belt

# \- \*\*Saturn's rings\*\* rendered as translucent multi-band geometry

# \- \*\*Phong-shaded planet spheres\*\* with day/night terminators and atmosphere rims

# \- \*\*Emissive Sun\*\* with procedural corona glow

# \- \*\*Orbital trails\*\* with fade-out effect

# \- \*\*2500 background stars\*\*

# \- \*\*Interactive camera\*\* — orbit, zoom, and follow individual planets

# \- \*\*Orbital inclinations\*\* for realistic 3D trajectories

# 

# \## Controls

# 

# | Key / Input     | Action                          |

# |-----------------|---------------------------------|

# | Left Drag       | Orbit camera                   |

# | Scroll           | Zoom in / out                  |

# | Space            | Pause / Resume simulation      |

# | R                | Reset simulation               |

# | T                | Toggle orbital trails          |

# | F                | Cycle follow target (planets)  |

# | ESC              | Quit                           |

# 

# \## Build

# 

# \### Requirements

# 

# \- C++11 or later

# \- OpenGL 3.3+

# \- \[GLFW](https://www.glfw.org/)

# \- \[GLEW](http://glew.sourceforge.net/)

# \- \[GLM](https://github.com/g-truc/glm)

# 

# \### Linux

# 

# ```bash

# g++ -std=c++11 -o solar\_system main.cpp -lGL -lGLEW -lglfw -lm

# ./solar\_system

# ```

# 

# \### Windows (MinGW)

# 

# ```bash

# g++ -std=c++11 -o solar\_system.exe main.cpp -lglew32 -lglfw3 -lopengl32 -lgdi32

# solar\_system.exe

# ```

# 

# \### macOS

# 

# ```bash

# g++ -std=c++11 -o solar\_system main.cpp -lGLEW -lglfw -framework OpenGL

# ./solar\_system

# ```

# 

# \## How It Works

# 

# The simulation uses \*\*Newtonian gravity\*\* with the equation:

# 

# ```

# F = G \* m1 \* m2 / (r² + ε²)

# ```

# 

# where `ε` is a softening factor to prevent singularities at close distances.

# 

# Each frame:

# 1\. Compute gravitational acceleration between all pairs of bodies

# 2\. Update velocities and positions using symplectic Euler integration

# 3\. Render each body as a point sprite with per-pixel sphere shading

# 

# Planets are initialized with calculated circular/inclined orbital velocities

# so they begin in stable orbits. The Moon orbits Earth, and Halley's Comet

# follows a highly eccentric inclined trajectory.

# 

# \## Project Structure

# 

# ```

# ├── main.cpp          # Complete simulation + rendering (single file)

# └── README.md

# ```

# 

# \## License

# 

# MIT License — free to use, modify, and distribute.

# ```

# 

# \## Topics / Tags (for GitHub repo settings)

# 

# ```

# opengl simulation physics solar-system gravity n-body cpp glfw

# space planets astronomy 3d real-time

# ```

