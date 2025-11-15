# Building HotPlug++

This guide will help you build and use the HotPlug++ plugin system.

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
    ├── libhotplug.a
    └── libsample_plugin.so (or .dll/.dylib)
```

## Integration into Your Project

### Option 1: Add as Subdirectory

```cmake
# In your CMakeLists.txt
add_subdirectory(external/HotPlugPP)
target_link_libraries(your_target PRIVATE hotplug)
```

### Option 2: Install and Use

```bash
# Build and install
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build .
sudo cmake --install .

# In your CMakeLists.txt
find_package(HotPlugPP REQUIRED)
target_link_libraries(your_target PRIVATE hotplug)
```

### Option 3: Header-Only Integration

Just copy the `include/` directory to your project and include the headers. Note that you'll need to compile `PluginLoader.cpp` as part of your build.

## Troubleshooting

### Linux: "cannot open shared object file"

Make sure the plugin path is correct or add the directory to `LD_LIBRARY_PATH`:

```bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib
./bin/host_app ./lib/libsample_plugin.so
```

### Windows: "The specified module could not be found"

Ensure all DLLs are in the same directory as the executable or in your PATH.

### macOS: "dyld: Library not loaded"

Set the runtime library path:

```bash
export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:./lib
./bin/host_app ./lib/libsample_plugin.dylib
```

### CMake Error: "Could not find a configuration file"

Make sure you're using CMake 3.15 or higher:

```bash
cmake --version
```

## Running Tests

Currently, the project includes example applications that serve as integration tests. To verify everything works:

```bash
cd build/bin
./host_app ./libsample_plugin.so

# You should see:
# - Plugin loading successfully
# - Regular update messages
# - Can test hot-reload by modifying and rebuilding the plugin
```

## Creating Your Own Plugin

1. Create a new plugin source file:

```cpp
// MyPlugin.cpp
#include "hotplug/IPlugin.hpp"

class MyPlugin : public hotplug::IPlugin {
    // Implement interface...
};

HOTPLUG_CREATE_PLUGIN(MyPlugin)
```

2. Add to CMakeLists.txt:

```cmake
add_library(my_plugin SHARED MyPlugin.cpp)
target_include_directories(my_plugin PRIVATE ${CMAKE_SOURCE_DIR}/include)
```

3. Build:

```bash
cmake --build . --target my_plugin
```

4. Load in your application:

```cpp
hotplug::PluginLoader loader;
loader.loadPlugin("./libmy_plugin.so");
```

## Performance Considerations

- **Hot-Reload Frequency**: By default, the example checks for plugin changes every second (60 frames at 60 FPS). Adjust based on your needs.
- **Plugin Loading**: Initial load is relatively slow (ms range), but subsequent updates are fast.
- **Memory**: Each plugin instance uses minimal memory overhead beyond your plugin's own allocations.

## Next Steps

- Read the [API Reference](API.md) for detailed interface documentation
- Check out [examples/](../examples/) for more usage patterns
- See [README.md](../README.md) for feature overview