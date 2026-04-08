# smite_editor

Windows/Linux ImGui tool for `game.smt`, `items.dat`, `monsters.dat`, `.maze`, `.lvl`, and wallset preview.

The main game CMake project targets Amiga; build this editor **standalone** from the repo root (or from `tools/smite_editor` with adjusted `-B` paths).

## Visual Studio 2022 (solution + projects)

From the **repository root**:

```powershell
cmake -S tools/smite_editor -B build/smite_editor_vs2022 -G "Visual Studio 17 2022" -A x64
```

Then open `build/smite_editor_vs2022/smite_editor.sln` in Visual Studio, pick **Release** or **Debug**, and build the **smite_editor** project. The executable is under `build/smite_editor_vs2022/Release/` or `Debug/` (or the configuration-specific folder CMake uses).

**File → Open → Folder…** on `tools/smite_editor` also works: Visual Studio will use `[CMakePresets.json](CMakePresets.json)` (preset **vs2022-x64**) and put the build tree in `build/smite_editor_vs2022`.

## Ninja (command line)

```bash
cmake -S tools/smite_editor -B build/smite_editor -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/smite_editor
```

Set **Data root** in the UI to your game `data` folder (e.g. copy of repo `data/` next to `smite.exe`, or the build tree `build/.../data` after a game build).

Dependencies (via FetchContent): GLFW 3.4, Dear ImGui `**v1.92.7-docking`** (the docking branch tag; Git must be available for the first configure). The editor uses **OpenGL 4.2** core (Windows/Linux) or **3.2** core (macOS) via `imgui_impl_opengl3`.

Layout and window positions are saved to `**smite_editor.ini`** in the current working directory when you quit. Delete that file to get the default dock layout again on the next launch.