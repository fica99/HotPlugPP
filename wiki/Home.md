# HotPlugPP Wiki

Welcome to the HotPlugPP Wiki — a lightweight, cross-platform plugin system in modern C++ with support for dynamic loading and hot-reloading of shared libraries.

## 🌟 Features

- 🔌 **Dynamic Loading**: Load and unload plugins at runtime
- 🔥 **Hot-Reloading**: Automatically detect and reload modified plugins without restart
- 🌐 **Cross-Platform**: Works on Windows (.dll), Linux (.so), and macOS (.dylib)
- 🎯 **Clean Interface**: Simple and intuitive plugin API
- 🛠️ **Modern C++**: Uses C++17 features
- 🚀 **Lightweight**: Minimal dependencies and overhead

## 📚 Contents

### Getting Started
- [[Build Instructions|BUILD]] — Build instructions for all platforms
- [[Tutorial|TUTORIAL]] — Step-by-step plugin creation guide

### Reference
- [[API Reference|API]] — Complete API documentation
- [[Contributing|CONTRIBUTING]] — Contribution guidelines

## 🚀 Quick Start

```bash
# Build
mkdir build && cd build
cmake .. && cmake --build .

# Run example
./bin/host_app ./bin/libsample_plugin.so
```

## 📋 Requirements

- CMake 3.15+
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)

## 🖥️ Platform Support

| Platform | Library Extension | Status |
|----------|-------------------|--------|
| Linux    | .so               | ✅ Supported |
| Windows  | .dll              | ✅ Supported |
| macOS    | .dylib            | ✅ Supported |

## 🔗 Links

- [GitHub Repository](https://github.com/fica99/HotPlugPP)
- [Issues](https://github.com/fica99/HotPlugPP/issues)
- [Pull Requests](https://github.com/fica99/HotPlugPP/pulls)
