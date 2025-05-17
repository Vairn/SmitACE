# Development Guide

## Setting Up Your Development Environment

### Prerequisites
1. Visual Studio Code
2. Amiga C Extension by Bartman
3. ACE compiler setup
4. Git

### Initial Setup
1. Clone the repository:
   ```bash
   git clone https://github.com/Vairn/SmitACE.git
   cd SmitACE
   ```

2. Install VSCode Extensions:
   - Amiga C/C++ Compile, Debug & Profile
   - C/C++ Extension Pack

3. Configure ACE:
   - Follow the [ACE installation guide](https://github.com/AmigaPorts/ACE/blob/master/docs/installing/compiler.md)
   - Ensure the AGA branch is properly set up

## Project Structure
- `src/`: Source code
- `include/`: Header files
- `res/`: Resource files
- `test/`: Test files
- `deps/`: Dependencies
- `docs/`: Documentation

## Building the Project

### Using VSCode
1. Open the project in VSCode
2. Use the Amiga C extension to build
3. The build output will be in the `build/` directory

### Using Command Line
```bash
mkdir build
cd build
cmake ..
make
```

## Development Workflow

### Code Style
- Follow the C coding standards
- Use meaningful variable and function names
- Comment complex logic
- Keep functions focused and small

### Testing
- Write tests for new features
- Run existing tests before committing
- Test on actual Amiga hardware when possible

### Version Control
- Use meaningful commit messages
- Create feature branches for new development
- Submit pull requests for major changes

## Debugging
1. Use the Amiga C extension's debug features
2. Enable debug logging when needed
3. Test on UAE emulator for quick iteration
4. Verify on real hardware for final testing

## Performance Considerations
- Optimize for Amiga hardware
- Be mindful of memory usage
- Use appropriate data structures
- Profile code when necessary

## Contributing
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## Resources
- [ACE Documentation](https://github.com/AmigaPorts/ACE)
- [Amiga Development Resources](https://wiki.amigaos.net/wiki/Main_Page)
- [AGA Programming Guide](https://wiki.amigaos.net/wiki/AGA_Programming_Guide) 