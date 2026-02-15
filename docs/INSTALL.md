# Installation Guide

This guide walks you through setting up the development environment for SmitACE on Windows.

## Prerequisites

| Requirement | Details |
|-------------|---------|
| **IDE** | Visual Studio Code |
| **Extension** | [Amiga C/C++ Compile, Debug & Profile](https://marketplace.visualstudio.com/items?itemName=BartmanAbyss.amiga-debug) by Bartman |
| **Build System** | CMake 3.14 or later |
| **ACE Framework** | Custom AGA branch (included as submodule) |

## Step-by-Step Installation

### 1. Clone the Repository

```bash
git clone --recursive https://github.com/Vairn/SmitACE.git
cd SmitACE
```

If you've already cloned without submodules:

```bash
git submodule update --init --recursive
```

### 2. Install the Amiga C Extension

1. Open VS Code
2. Open the Extensions view (`Ctrl+Shift+X`)
3. Search for "Amiga C" or "Amiga Debug"
4. Install **Amiga C/C++ Compile, Debug & Profile** by Bartman

The extension provides the GCC toolchain and Amiga development libraries.

### 3. ACE Compiler Setup

The project uses the ACE framework as a submodule in `deps/ACE`. Follow the [ACE installation instructions](https://github.com/AmigaPorts/ACE/blob/master/docs/installing/compiler.md) to ensure your toolchain is configured correctly.

The Bartman extension typically handles this when building from VS Code.

### 4. Configure the Build

Create a build directory and configure:

```bash
mkdir build
cd build
cmake ..
```

For a release build (default):

```bash
cmake .. -DACE_DEBUG=OFF -DGAME_DEBUG=OFF
```

For debugging:

```bash
cmake .. -DACE_DEBUG=ON -DGAME_DEBUG=ON
```

### 5. Build the Project

From the `build` directory:

```bash
cmake --build .
```

Or use VS Code's build task (`Ctrl+Shift+B`).

### 6. Prepare Runtime Data

Copy the `data` folder to your build output directory. The data folder contains game assets (graphics, levels, sounds) required at runtime.

The build output is typically in `build/unknown/` (or similar, depending on your CMake kit).

## Verifying the Installation

After building:

1. Locate `smite.exe` (or `smite` on native Amiga) in the build directory
2. Ensure the `data` folder is alongside the executable
3. Run the game via the Amiga C extension's debug/run commands, or in UAE/FS-UAE

## Troubleshooting

### Submodule Issues

If `deps/ACE` is empty or build fails with missing ACE headers:

```bash
git submodule update --init --recursive
```

### Missing Data Folder

The game will fail to start without the `data` folder. Ensure it's copied from the project root to the build output directory.

### CMake Kit Selection

In VS Code, select the appropriate CMake kit (e.g., "Amiga GCC" or "bartman_gcc_support") from the status bar before configuring.
