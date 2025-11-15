# Contributing to HotPlug++

Thank you for considering contributing to HotPlug++! This document provides guidelines for contributing to the project.

## Code of Conduct

- Be respectful and constructive
- Focus on what is best for the community
- Show empathy towards other contributors

## How to Contribute

### Reporting Bugs

Before creating bug reports, please check existing issues to avoid duplicates. When creating a bug report, include:

- **Clear title and description**
- **Steps to reproduce** the issue
- **Expected behavior** vs **actual behavior**
- **Environment details**: OS, compiler version, CMake version
- **Code samples** if applicable

### Suggesting Enhancements

Enhancement suggestions are tracked as GitHub issues. When creating an enhancement suggestion, include:

- **Clear title and description**
- **Use case** - why is this enhancement needed?
- **Proposed solution** - how would you implement it?
- **Alternatives considered**

### Pull Requests

1. **Fork the repository** and create your branch from `main`
2. **Make your changes** following the coding standards
3. **Test your changes** thoroughly
4. **Update documentation** if needed
5. **Submit a pull request**

## Development Setup

```bash
# Clone your fork
git clone https://github.com/YOUR_USERNAME/HotPlugPP.git
cd HotPlugPP

# Create a build directory
mkdir build && cd build

# Configure and build
cmake ..
cmake --build .

# Test your changes
./bin/host_app ./bin/libsample_plugin.so
```

## Coding Standards

### C++ Style

- **C++ Standard**: C++17 or newer
- **Naming Conventions**:
  - Classes: `PascalCase` (e.g., `PluginLoader`)
  - Functions: `camelCase` (e.g., `loadPlugin`)
  - Member variables: `m_camelCase` (e.g., `m_pluginInfo`)
  - Constants: `UPPER_CASE` (e.g., `MAX_PLUGINS`)
- **Formatting**:
  - Indentation: 4 spaces (no tabs)
  - Braces: Opening brace on same line
  - Line length: Prefer < 100 characters
- **Comments**:
  - Use `///` for documentation comments
  - Explain "why", not "what"
  - Keep comments up to date

### Example

```cpp
class PluginManager {
public:
    /// Load a plugin from the specified path
    /// @param path Path to the plugin library
    /// @return true if successful, false otherwise
    bool loadPlugin(const std::string& path) {
        if (path.empty()) {
            return false;
        }
        // Implementation...
    }

private:
    std::string m_currentPath;
    int m_loadCount;
};
```

## Testing

- **Build tests**: Ensure the project builds on Linux, Windows, and macOS
- **Runtime tests**: Test plugin loading, unloading, and hot-reloading
- **Edge cases**: Test error conditions and edge cases
- **Memory**: Check for memory leaks with valgrind or similar tools

### Running Tests

```bash
# Build and run examples
cd build
cmake --build .

# Test sample plugin
./bin/host_app ./bin/libsample_plugin.so

# Test math plugin
./bin/host_app ./bin/libmath_plugin.so

# Test hot-reload
# 1. Start host_app with a plugin
# 2. Modify the plugin source
# 3. Rebuild the plugin
# 4. Verify hot-reload occurs
```

## Documentation

- **Code comments**: Document public APIs
- **README**: Update if adding major features
- **API docs**: Update API.md for interface changes
- **Tutorials**: Add examples for new features

## Commit Messages

Write clear, descriptive commit messages:

```
Add hot-reload callback feature

- Add setReloadCallback() method to PluginLoader
- Call callback after successful hot-reload
- Update documentation and examples
- Add test for callback invocation
```

Format:
- First line: Brief summary (50 chars or less)
- Blank line
- Detailed description if needed
- List specific changes

## Pull Request Process

1. **Update documentation** to reflect changes
2. **Test on multiple platforms** if possible
3. **Ensure CI passes** (if available)
4. **Request review** from maintainers
5. **Address feedback** promptly
6. **Squash commits** if requested

## Project Areas

Areas where contributions are especially welcome:

### Core Features
- Multi-plugin support (loading multiple plugins)
- Plugin dependency management
- Thread-safety improvements
- Performance optimizations

### Platform Support
- Testing on macOS
- Testing on Windows (MSVC and MinGW)
- ARM architecture support

### Examples
- More example plugins
- GUI host application example
- Multi-plugin example
- Real-world use case examples

### Documentation
- Video tutorials
- Blog posts
- Translations
- API reference improvements

### Build System
- Package manager support (vcpkg, conan)
- Install targets
- Find package config

## Questions?

Feel free to open an issue with your question. We're happy to help!

## License

By contributing, you agree that your contributions will be licensed under the same license as the project (see LICENSE file).

## Recognition

Contributors will be acknowledged in the project README. Thank you for helping make HotPlug++ better! 🎉