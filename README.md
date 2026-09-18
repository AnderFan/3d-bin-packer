# 3D Pallet Packing Visualizer & Optimizer

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Raylib](https://img.shields.io/badge/Render-Raylib-red.svg)](https://www.raylib.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

An interactive 3D container & pallet loading optimization software written in modern **C++20** with real-time rendering and immediate-mode GUI powered by **[Raylib](https://www.raylib.com/)** and **[Raygui](https://github.com/raysan5/raygui)**.
/<img width="1070" height="515" alt="изображение" src="https://github.com/user-attachments/assets/b592c367-32bc-4e1c-96bf-391909148daa" />

<img width="1051" height="496" alt="изображение" src="https://github.com/user-attachments/assets/40ab8447-9933-4a97-8cdd-728b28a210d1" />

---

## Overview

The visualizer bridges computational packing optimization and interactive 3D logistics inspection:
1. **Interactive Configurator**: Configure pallet dimensions, weight capacities, and heterogeneous box batches with custom 3D rotation permissions.
2. **Packing Heuristic Solver**: Partition space using dynamic candidate zones and volume/center-of-mass heuristics.
3. **Interactive 3D Inspection**: Inspect the resulting layout in a 3D perspective viewport with orbit camera controls, height indicators, and diagnostic telemetry.

---

## Features

- **Interactive GUI**:
  - Full configuration of pallet bounds ($X, Y, Z$) and gross weight limits.
  - Multi-box inventory management: dimensions, individual unit weight, quantity, and 6-DOF rotation toggles.
  - Infinite cargo generator mode (`Max.`) to fill pallets to theoretical capacity using single-template items.
- **Dual Optimization Strategies**:
  - **Center of Mass (Stability)**: Prioritizes lower and centralized mass distribution for road and maritime transport safety.
  - **Maximum Volume (Capacity)**: Maximizes cubic volume density and packing factor.
  - **Full Layer Enforcement**: Optional restriction to prune incomplete top layers for uniform stacking.
- **Dynamic 3D Geometry Solver**:
  - **Zone Splitting & Merging**: Dissects remaining void volumes along 3D bounding planes and merges adjacent sub-zones to combat spatial fragmentation.
  - **Maximal Empty Box (MEB) Fallback**: Scans the heightmap grid to discover overlooked contiguous packing spaces.
  - **Auto-Centering**: Automatically centers the placed cargo footprint on the pallet floor.
---

## Tech Stack & Dependencies

- **Language**: C++20 (GCC 11+, Clang 13+, or MSVC 2019+)
- **Build System**: CMake 3.20+
- **Graphics & Windowing**: [Raylib 5.0+](https://www.raylib.com/)
- **UI Engine**: [Raygui 4.0+](https://github.com/raysan5/raygui)

---

## Getting Started

### Prerequisites

Ensure you have CMake, a C++20 compatible compiler, and Raylib installed on your machine.

#### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential cmake libraylib-dev libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev
```

#### macOS (via Homebrew)
```bash
brew install cmake raylib
```

#### Windows (vcpkg)
```powershell
vcpkg install raylib:x64-windows
```

---

### Building from Source

```bash
# 1. Clone the repository
git clone https://github.com/yourusername/3d-pallet-packer.git
cd 3d-pallet-packer

# 2. Configure project via CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Build executable
cmake --build build --config Release -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
```

The compiled binary will be located in the `build/` directory (e.g., `./build/3dBox` or `./build/Release/3dBox.exe`).

---
### 3D Viewport Navigation
| Input | Action |
| :--- | :--- |
| **Left Mouse Button (Hold & Drag)** | Orbit camera around the pallet center |
| **Mouse Wheel (Scroll)** | Zoom in / Zoom out |
| **Escape (ESC)** | Return to the input configuration menu |

---

## 📐 Architecture & Pipeline

```text
       ┌───────────────┐
       │   UI (Menu)   │ ◄── User Input (Pallet, Boxes, Settings)
       └───────┬───────┘
               │
               ▼
       ┌───────────────┐
       │   3D Render   │ 
       └───────────────┘
               │
               ▼
       ┌───────────────┐
       │ pallet_handle │ ◄── Main Solver Orchestrator
       └───────┬───────┘
               ├─────────────────────────┬─────────────────────────┐
               ▼                         ▼                         ▼
      ┌─────────────────┐       ┌─────────────────┐       ┌─────────────────┐
      │   pal_box.cpp   │       │  pal_zone.cpp   │       │   pal_meb.cpp   │
      │ Scoring/Placing │       │ Split / Merge   │       │ Max Empty Box   │
      └─────────────────┘       └─────────────────┘       └─────────────────┘

```

## 📄 License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.
