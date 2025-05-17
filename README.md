[![MIT][license-shield]][license-text]
# SmitACE

Smite Ported To C
Using A Custom AGA branch of [ACE](https://github.com/Vairn/ACE/tree/AGA)

Based on my BlitzGameJam Entry [My Life Sucks](https://vairn.itch.io/my-life-sucks)

[github](https://github.com/Vairn/Smite/tree/PostJam)

## Overview
SmitACE is a C port of the game Smite, built using a custom AGA branch of the ACE framework. This project aims to bring the game to the Amiga platform with AGA support.

## Project Structure
```
SmitACE/
├── src/           # Source code
│   ├── game/      # Core game logic
│   ├── Gfx/       # Graphics and rendering
│   ├── items/     # Game items and inventory
│   ├── maze/      # Maze generation and management
│   └── misc/      # Miscellaneous utilities
├── include/       # Header files
├── res/          # Resource files
├── test/         # Test files
└── deps/         # Dependencies
```

## Prerequisites
- Visual Studio Code
- Amiga C Extension by Bartman ([Amiga C/C++ Compile, Debug & Profile](https://marketplace.visualstudio.com/items?itemName=BartmanAbyss.amiga-debug))
- ACE compiler setup (see Installation section)

## Installation
1. Clone the repository
2. Install the Amiga C Extension in VSCode
3. Follow the [ACE installation instructions](https://github.com/AmigaPorts/ACE/blob/master/docs/installing/compiler.md)
4. Open the project in VSCode
5. Build the project (it will create a build folder)
6. Copy the data folder to the build directory

## Building
The project uses CMake as its build system. The build process will:
1. Create a build directory
2. Generate necessary build files
3. Compile the project
4. Place the executable in the build directory

## Development
- Main game logic is in `src/game/`
- Graphics and rendering code is in `src/Gfx/`
- Item system is in `src/items/`
- Maze generation is in `src/maze/`

## License
This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.

[license-text]: https://opensource.org/license/mit/
[license-shield]: https://img.shields.io/badge/license-MIT-brightgreen
