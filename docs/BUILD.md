# Building HotPlugPP

Detailed build instructions for all platforms.

## Prerequisites

### Required
- CMake 3.15 or higher
- C++17 compatible compiler:
  - GCC 7+ (Linux)
  - Clang 5+ (macOS/Linux)
  - MSVC 2017+ (Windows)
  - MinGW-w64 (Windows)

### Optional
- Git (for cloning the repository)
- IDE with CMake support (CLion, Visual Studio, VS Code)

## Building on Linux/macOS

```bash
# Clone the repository
git clone https://github.com/fica99/HotPlugPP.git
cd HotPlugPP

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
cmake --build .

# Or use make directly
make -j$(nproc)

# Run the example
./bin/host_app ./bin/libsample_plugin.so
```

## Building on Windows

### Using Visual Studio

```cmd
# Open Developer Command Prompt for VS

# Clone the repository
git clone https://github.com/fica99/HotPlugPP.git
cd HotPlugPP

# Create build directory
mkdir build
cd build

# Configure for Visual Studio (example for VS 2022)
cmake .. -G "Visual Studio 17 2022"

# Build
cmake --build . --config Release

# Run the example
.\bin\Release\host_app.exe .\bin\Release\sample_plugin.dll
```

### Using MinGW

```cmd
# Clone the repository
git clone https://github.com/fica99/HotPlugPP.git
cd HotPlugPP

# Create build directory
mkdir build
cd build

# Configure for MinGW
cmake .. -G "MinGW Makefiles"

# Build
cmake --build .

# Run the example
.\bin\host_app.exe .\bin\libsample_plugin.dll
```

## Build Options

### Debug Build

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

### Release Build

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Specify Compiler

```bash
# Use Clang
cmake .. -DCMAKE_CXX_COMPILER=clang++

# Use specific GCC version
cmake .. -DCMAKE_CXX_COMPILER=g++-11
```

## Output Structure

After building, the output directory structure will be:

```
build/
├── bin/
│   ├── host_app (or host_app.exe on Windows)
│   └── libsample_plugin.so (or .dll/.dylib)
└── lib/
    ├── libhotplugpp.a
    └── libsample_plugin.so (or .dll/.dylib)
```

## Integration into Your Project

### Option 1: Add as Subdirectory

```cmake
# In your CMakeLists.txt
add_subdirectory(external/HotPlugPP)
target_link_libraries(your_target PRIVATE hotplugpp)
```

### Option 2: Install and Use

```bash
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build .
sudo cmake --install .
```

After installing HotPlugPP, add the following to your project's `CMakeLists.txt` to link against the library:

```cmake
find_package(HotPlugPP REQUIRED)
target_link_libraries(your_target PRIVATE hotplugpp)
```

### Option 3: Header-Only Integration

Copy the `include/` directory to your project and compile `src/PluginLoader.cpp`.

## Troubleshooting

### Linux: "cannot open shared object file"

Set `LD_LIBRARY_PATH`:
```bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib
./bin/host_app ./lib/libsample_plugin.so
```

### Windows: "The specified module could not be found"

Ensure all DLLs are in the same directory as the executable or in your PATH.

### macOS: "dyld: Library not loaded"

Set runtime library path:
```bash
export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:./lib
./bin/host_app ./lib/libsample_plugin.dylib
```

### CMake Error: "Could not find a configuration file"

Verify CMake version:
```bash
cmake --version  # Should be 3.15+
```

## Next Steps

- See [TUTORIAL.md](TUTORIAL.md) for creating your first plugin
- See [API.md](API.md) for complete API documentation
- Check out [../examples/](../examples/) for usage examples