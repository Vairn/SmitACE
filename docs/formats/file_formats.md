# File Formats Documentation

## Wallset Format

### Overview
The wallset format is used to store wall graphics and their properties for the game. It consists of a main data file and associated graphics files.

### Main Wallset File Structure
The main wallset file (`.wst`) has the following binary structure:

```
[Header] (3 bytes)
    - Magic number/identifier

[Palette Section]
    - Palette Size (1 byte)
    - Palette Data (Palette Size * 3 bytes)
        - RGB values for each color

[Tileset Section]
    - Total Tileset Count (2 bytes)
    - Tileset Count (1 byte)
    - For each tileset:
        - Wallset Count (1 byte)
        - For each wall:
            - Type (1 byte)
            - Set Index (1 byte)
            - Location [2] (2 bytes)
            - Screen [2] (4 bytes)
            - X Position (2 bytes)
            - Y Position (2 bytes)
            - Width (2 bytes)
            - Height (2 bytes)
```

### Associated Files
For each tileset, there are two associated files:
1. Graphics file: `[basename]_[number].pln`
   - Contains the actual wall graphics
   - Uses the palette from the main file
   - Stored in planar format

2. Mask file: `[basename]_[number].msk`
   - Contains collision/transparency masks
   - 1-bit per pixel
   - Used for collision detection

### Data Types
- `UBYTE`: 1 byte, unsigned
- `BYTE`: 1 byte, signed
- `UWORD`: 2 bytes, unsigned
- `WORD`: 2 bytes, signed

### Example
For a wallset file named `walls.wst`, the associated files would be:
```
walls.wst      # Main wallset data
walls_0.pln    # First tileset graphics
walls_0.msk    # First tileset mask
walls_1.pln    # Second tileset graphics
walls_1.msk    # Second tileset mask
```

### Usage
```c
// Load a wallset
tWallset *pWallset = wallsetLoad("walls.wst");

// Use the wallset
// ... game code ...

// Clean up when done
wallsetDestroy(pWallset);
```

### Memory Management
- The wallset loader allocates memory for:
  - Palette data
  - Tileset structures
  - Graphics bitmaps
  - Mask bitmaps
- All memory is properly freed by `wallsetDestroy()`

### Best Practices
1. Always check if wallset loading was successful
2. Clean up wallset resources when no longer needed
3. Use appropriate error handling
4. Consider memory constraints when creating wallsets

### Limitations
- Maximum palette size: 256 colors
- Maximum tileset count: 65535
- Maximum wall dimensions: 65535x65535 pixels
- File paths limited to 256 characters

### Tools
- Use the provided wallset editor to create and modify wallsets
- Convert images to planar format before creating wallsets
- Validate wallset files before using them in the game

## Bitmap Format

### Overview
The game uses several bitmap formats for different purposes:
- `.bm`: Raw bitmap data in planar format
- `.png`: Source images (converted to planar format)
- `.iff`: IFF ILBM format for Amiga compatibility

### Planar Bitmap Format (`.bm`)
The planar bitmap format is optimized for Amiga hardware and has the following structure:

```
[Header]
    - Width (2 bytes)
    - Height (2 bytes)
    - Number of bitplanes (1 byte)
    - Flags (1 byte)

[Bitmap Data]
    - Planar data for each bitplane
    - Each bitplane contains 1 bit per pixel
    - Data is organized in rows
```

### IFF ILBM Format (`.iff`)
The IFF ILBM format is the standard Amiga image format:
- Supports multiple bitplanes
- Includes palette information
- Compatible with Amiga graphics software

### Usage
```c
// Load a bitmap
tBitMap *pBitmap = bitmapCreateFromPath("image.bm", 0);

// Use the bitmap
// ... game code ...

// Clean up
bitmapDestroy(pBitmap);
```

## Palette Format

### Overview
The game uses several palette formats:
- `.pal`: Raw palette data
- `.plt`: Palette table
- `.gpl`: GIMP palette format (for development)

### Raw Palette Format (`.pal`)
The raw palette format has the following structure:
```
[Header]
    - Number of colors (1 byte)

[Color Data]
    - For each color:
        - Red (1 byte)
        - Green (1 byte)
        - Blue (1 byte)
```

### Palette Table Format (`.plt`)
The palette table format is used for quick palette switching:
```
[Header]
    - Number of palettes (1 byte)

[Palette Data]
    - For each palette:
        - Number of colors (1 byte)
        - Color data (RGB triplets)
```

### Usage
```c
// Load a palette
tPalette *pPalette = paletteLoad("game.pal");

// Use the palette
// ... game code ...

// Clean up
paletteDestroy(pPalette);
```

## Audio Format

### Overview
The game uses several audio formats:
- `.mod`: Protracker module format
- `.smp`: Sample data
- `.mus`: Music data

### Module Format (`.mod`)
The Protracker module format is the standard Amiga music format:
- Supports up to 4 channels
- Includes instrument samples
- Pattern-based sequencing

### Sample Format (`.smp`)
Raw sample data format:
```
[Header]
    - Sample length (4 bytes)
    - Sample rate (2 bytes)
    - Flags (1 byte)

[Sample Data]
    - 8-bit signed PCM data
```

### Usage
```c
// Load a module
tModule *pModule = moduleLoad("theme.mod");

// Play the module
modulePlay(pModule);

// Clean up
moduleDestroy(pModule);
```

## Best Practices
1. Always use the appropriate format for the target platform
2. Convert source images to planar format before use
3. Optimize palette usage for Amiga hardware
4. Use appropriate compression for audio files
5. Validate file formats before loading

## Tools
- Use GIMP for image editing and palette creation
- Use Protracker for music creation
- Use the provided conversion tools for format conversion
- Validate files before using them in the game 