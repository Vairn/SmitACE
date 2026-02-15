# How to Fix 68020 Compilation Issue

## Problem
Even though `cmake-kits.json` has `"M68K_CPU": "68020"`, the build is still compiling with `-m68000` because CMake is using a cached value.

## Solution

You need to reconfigure CMake to pick up the new CPU setting. Here are the options:

### Option 1: Delete CMake Cache (Recommended)
1. Close any running builds
2. Delete the `build/CMakeCache.txt` file
3. In VSCode, use `Ctrl+Shift+P` and select **"CMake: Delete Cache and Reconfigure"**
4. Select your kit again ("GCC Bartman m68k Win32" which has 68020)

### Option 2: Manual Reconfigure
1. In VSCode, use `Ctrl+Shift+P`
2. Select **"CMake: Configure"**
3. Or select **"CMake: Delete Cache and Reconfigure"** to force a fresh configuration

### Option 3: Command Line
From the project root:
```bash
cd build
cmake .. -DM68K_CPU=68020 -G Ninja
```

### Option 4: Force in CMakeLists.txt (Not Recommended)
You could add this to your CMakeLists.txt to force it:
```cmake
set(M68K_CPU "68020" CACHE STRING "Target CPU model" FORCE)
```

But Option 1 or 2 is better since it respects your kit configuration.

## Verification

After reconfiguring, check:
1. `build/CMakeCache.txt` should have: `M68K_CPU:STRING=68020`
2. `build/build.ninja` should have: `-m68020` instead of `-m68000`

## Why This Happens

CMake caches variables. When you set `M68K_CPU` in `cmake-kits.json`, it passes `-DM68K_CPU=68020` during configuration. However, if `CMakeCache.txt` already exists with `M68K_CPU=68000`, CMake will use the cached value unless you:
- Delete the cache
- Use `FORCE` in the `set()` command
- Explicitly pass `-DM68K_CPU=68020` via command line

The `cmake-kits.json` setting works on **initial configuration**, but if you've configured before, you need to reconfigure.

