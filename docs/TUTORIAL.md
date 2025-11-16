# HotPlugPP Tutorial: Creating Your First Plugin

This tutorial guides you through creating a custom plugin for HotPlugPP from scratch.

## Prerequisites

- HotPlugPP built and working (see [BUILD.md](BUILD.md))
- Basic C++ knowledge
- Text editor or IDE

## Tutorial Overview

We'll create a "Greeter Plugin" that:
1. Greets the user on load
2. Counts updates
3. Says goodbye on unload

## Step 1: Create Plugin Source File

Create a new file `examples/greeter_plugin/GreeterPlugin.cpp`:

```cpp
#include "hotplugpp/IPlugin.hpp"
#include <iostream>
#include <string>

class GreeterPlugin : public hotplugpp::IPlugin {
public:
    GreeterPlugin() : m_updateCount(0) {
        std::cout << "[GreeterPlugin] Instance created" << std::endl;
    }

    ~GreeterPlugin() override {
        std::cout << "[GreeterPlugin] Instance destroyed" << std::endl;
    }

    bool onLoad() override {
        std::cout << "[GreeterPlugin] Hello! I'm loading now." << std::endl;
        m_updateCount = 0;
        return true;
    }

    void onUnload() override {
        std::cout << "[GreeterPlugin] Goodbye! I was updated " 
                  << m_updateCount << " times." << std::endl;
    }

    void onUpdate(float deltaTime) override {
        m_updateCount++;
        
        // Print every 120 frames (every 2 seconds at 60 FPS)
        if (m_updateCount % 120 == 0) {
            std::cout << "[GreeterPlugin] Still here! Update #" 
                      << m_updateCount << std::endl;
        }
    }

    const char* getName() const override {
        return "GreeterPlugin";
    }

    hotplugpp::Version getVersion() const override {
        return hotplugpp::Version(1, 0, 0);
    }

    const char* getDescription() const override {
        return "A friendly plugin that greets you";
    }

private:
    int m_updateCount;
};

// Export the plugin - this is required!
HOTPLUGPP_CREATE_PLUGIN(GreeterPlugin)
```

## Step 2: Add to Build System

Edit `examples/CMakeLists.txt` and add:

```cmake
# Greeter Plugin
add_library(greeter_plugin SHARED
    greeter_plugin/GreeterPlugin.cpp
)

target_include_directories(greeter_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(greeter_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "greeter_plugin"
)

if(WIN32)
    set_target_properties(greeter_plugin PROPERTIES SUFFIX ".dll")
else()
    set_target_properties(greeter_plugin PROPERTIES PREFIX "lib" SUFFIX ".so")
endif()

add_custom_command(TARGET greeter_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:greeter_plugin>
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<TARGET_FILE_NAME:greeter_plugin>
    COMMENT "Copying greeter_plugin to bin directory"
)
```

## Step 3: Build Your Plugin

```bash
cd build
cmake --build . --target greeter_plugin
```

You should see output like:
```
[100%] Built target greeter_plugin
Copying greeter_plugin to bin directory
```

## Step 4: Run Your Plugin

```bash
cd bin
./host_app ./libgreeter_plugin.so
```

Expected output:
```
=== HotPlugPP Example Host Application ===

Loading plugin from: ./libgreeter_plugin.so
[GreeterPlugin] Instance created
[GreeterPlugin] Hello! I'm loading now.
Plugin loaded successfully: GreeterPlugin v1.0.0

Plugin loaded successfully!
  Name: GreeterPlugin
  Version: 1.0.0
  Description: A friendly plugin that greets you

Starting update loop (hot-reload monitoring enabled)...
You can modify and recompile the plugin to see hot-reload in action!

[GreeterPlugin] Still here! Update #120
[GreeterPlugin] Still here! Update #240
```

## Step 5: Test Hot-Reload

Keep the host app running, then in another terminal:

1. **Edit the plugin** - Change the greeting message:
```cpp
bool onLoad() override {
    std::cout << "[GreeterPlugin] Hello again! I've been reloaded!" << std::endl;
    m_updateCount = 0;
    return true;
}
```

2. **Rebuild just the plugin**:
```bash
cd build
cmake --build . --target greeter_plugin
```

3. **Watch the host app terminal** - You should see:
```
[GreeterPlugin] Goodbye! I was updated XXX times.
[GreeterPlugin] Instance destroyed
Plugin file modified, reloading...
[GreeterPlugin] Instance created
[GreeterPlugin] Hello again! I've been reloaded!
Plugin loaded successfully: GreeterPlugin v1.0.0

*** Plugin has been reloaded! ***
```

Congratulations! You've created your first hot-reloadable plugin!

## Advanced: Adding State Management

Let's enhance the plugin to keep custom state:

```cpp
class GreeterPlugin : public hotplugpp::IPlugin {
public:
    GreeterPlugin() : m_updateCount(0), m_totalTime(0.0f) {}

    bool onLoad() override {
        std::cout << "[GreeterPlugin] Hello! Starting fresh." << std::endl;
        // State is reset on reload
        m_updateCount = 0;
        m_totalTime = 0.0f;
        return true;
    }

    void onUpdate(float deltaTime) override {
        m_updateCount++;
        m_totalTime += deltaTime;
        
        if (m_updateCount % 120 == 0) {
            std::cout << "[GreeterPlugin] Update #" << m_updateCount 
                      << " (running for " << m_totalTime << "s)" << std::endl;
        }
    }

    void onUnload() override {
        std::cout << "[GreeterPlugin] Goodbye! Stats:" << std::endl;
        std::cout << "  Updates: " << m_updateCount << std::endl;
        std::cout << "  Runtime: " << m_totalTime << "s" << std::endl;
    }

    // ... rest of the implementation

private:
    int m_updateCount;
    float m_totalTime;
};
```

## Best Practices

### 1. Initialize in onLoad(), Clean up in onUnload()

```cpp
bool onLoad() override {
    m_texture = loadTexture("texture.png");
    return m_texture != nullptr;  // Return false if init fails
}

void onUnload() override {
    delete m_texture;
    m_texture = nullptr;
}
```

### 2. Keep onUpdate() Fast

```cpp
void onUpdate(float deltaTime) override {
    // Good: Fast operation
    m_position += m_velocity * deltaTime;
    
    // Bad: Don't do expensive operations every frame
}
```

### 3. Handle Hot-Reload Gracefully

State is NOT preserved across reloads. If you need persistent state:
- Save to a file in `onUnload()`
- Load from a file in `onLoad()`
- Use a separate state manager in the host application

### 4. Version Your Plugins

```cpp
hotplugpp::Version getVersion() const override {
    return hotplugpp::Version(1, 2, 3);  // major.minor.patch
}
```

Increment:
- **Major**: Breaking changes
- **Minor**: New features, backwards compatible
- **Patch**: Bug fixes

## Troubleshooting

### Plugin doesn't load
- Check file path is correct
- Verify file has execute permissions
- Ensure all dependencies are available

### "Failed to find plugin factory functions"
Make sure you included:
```cpp
HOTPLUGPP_CREATE_PLUGIN(YourPluginClassName)
```

### Hot-reload doesn't work
- Ensure file was actually rebuilt
- Verify `checkAndReload()` is being called
- Check plugin file isn't locked by another process

## Next Steps

- Explore the examples: `sample_plugin` and `math_plugin`
- Read [API.md](API.md) for complete interface documentation
- Build something real!

Happy plugin development! 🔌