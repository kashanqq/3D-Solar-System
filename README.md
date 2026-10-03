# 3D Solar System

A real-time 3D simulation of the solar system written in C++ and OpenGL. The Sun, eight planets and the Moon attract each other through N-body gravity, and you can fly a free camera around them while the simulation runs.

## Features

- **N-body gravity:** every body attracts every other body, nothing moves on a pre-baked path
- **Leapfrog integration (kick-drift-kick):** a symplectic integrator that keeps orbits stable over long runs
- **Adaptive substeps:** the number of physics steps per frame grows with the simulation speed, so fast-forwarding does not break the orbits
- **Orbit trails:** each body leaves a line trail of its recent positions
- **3D models:** planets are loaded from FBX files through Assimp
- **Separate shaders** for planets, the Sun and the trails

## Controls

| Key | Action |
| --- | --- |
| `W` `A` `S` `D` | Move the camera |
| Mouse | Look around |
| `Left Shift` | Move faster |
| `[` / `]` | Slow down / speed up the simulation |
| `Esc` | Quit |

## Tech

C++20, OpenGL 3.3, GLFW, GLAD, GLM, Assimp, CMake, vcpkg

## Build

Requirements: CMake 3.22+, a C++20 compiler and [vcpkg](https://github.com/microsoft/vcpkg) installed in `~/vcpkg` (it provides Assimp).

```bash
git clone https://github.com/kashanqq/3D-Solar-System.git
cd 3D-Solar-System
cmake -B build
cmake --build build
./Solar-System
```

The executable is placed in the project root, because it loads `shaders/` and `models/` by relative path.

### Models

The planet models are not stored in the repository. Put FBX files into a `models/` folder in the project root with these names:

`Sun.fbx`, `Mercury.fbx`, `Venus.fbx`, `Earth.fbx`, `Moon.fbx`, `Mars.fbx`, `Jupiter.fbx`, `Saturn.fbx`, `Uranus.fbx`, `Neptune.fbx`

## Project structure

```
main.cpp        simulation, camera and rendering loop
shaders/        GLSL shaders for planets, the Sun and trails
external/       GLFW, GLAD, GLM and the Shader/Model/Mesh headers
```

## Notes

Masses, distances and sizes are scaled for a readable picture and are not real-world values.
