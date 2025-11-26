#include "hotplugpp/plugin_loader.hpp"
#include "hotplugpp/logger.hpp"

#include <utility>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <sys/stat.h>

namespace hotplugpp {

PluginLoader::PluginLoader() = default;

PluginLoader::~PluginLoader() {
    unloadPlugin();
}

bool PluginLoader::loadPlugin(const std::string& path) {
    // Unload existing plugin if any
    if (isLoaded()) {
        unloadPlugin();
    }

    // Load the shared library
    LibraryHandle handle = loadLibrary(path);
    if (!handle) {
        HOTPLUGPP_LOG_ERROR("Failed to load library: " + path);
        HOTPLUGPP_LOG_ERROR("Error: " + getLastError());
        return false;
    }

    // Get the factory functions
    CreatePluginFunc createFunc = reinterpret_cast<CreatePluginFunc>(
        getFunction(handle, "createPlugin"));
    DestroyPluginFunc destroyFunc = reinterpret_cast<DestroyPluginFunc>(
        getFunction(handle, "destroyPlugin"));

    if (!createFunc || !destroyFunc) {
        HOTPLUGPP_LOG_ERROR("Failed to find plugin factory functions in: " + path);
        HOTPLUGPP_LOG_ERROR("Error: " + getLastError());
        unloadLibrary(handle);
        return false;
    }

    // Create plugin instance
    IPlugin* plugin = createFunc();
    if (!plugin) {
        HOTPLUGPP_LOG_ERROR("Failed to create plugin instance from: " + path);
        unloadLibrary(handle);
        return false;
    }

    // Initialize plugin
    if (!plugin->onLoad()) {
        HOTPLUGPP_LOG_ERROR("Plugin initialization failed: " + path);
        destroyFunc(plugin);
        unloadLibrary(handle);
        return false;
    }

    // Store plugin info
    m_pluginInfo.path = path;
    m_pluginInfo.handle = handle;
    m_pluginInfo.instance = plugin;
    m_pluginInfo.createFunc = createFunc;
    m_pluginInfo.destroyFunc = destroyFunc;
    m_pluginInfo.lastModified = getFileModificationTime(path);
    m_pluginInfo.isLoaded = true;

    HOTPLUGPP_LOG_INFO("Plugin loaded successfully: " + std::string(plugin->getName()) + " v" +
                       plugin->getVersion().toString());

    return true;
}

void PluginLoader::unloadPlugin() {
    if (!isLoaded()) {
        return;
    }

    // Call plugin cleanup
    if (m_pluginInfo.instance) {
        m_pluginInfo.instance->onUnload();

        // Destroy plugin instance
        if (m_pluginInfo.destroyFunc) {
            m_pluginInfo.destroyFunc(m_pluginInfo.instance);
        }
        m_pluginInfo.instance = nullptr;
    }

    // Unload library
    if (m_pluginInfo.handle) {
        unloadLibrary(m_pluginInfo.handle);
        m_pluginInfo.handle = nullptr;
    }

    m_pluginInfo.isLoaded = false;
    m_pluginInfo.createFunc = nullptr;
    m_pluginInfo.destroyFunc = nullptr;
}

bool PluginLoader::checkAndReload() {
    if (!isLoaded()) {
        return false;
    }

    auto currentModTime = getFileModificationTime(m_pluginInfo.path);

    // Check if file has been modified
    if (currentModTime > m_pluginInfo.lastModified) {
        HOTPLUGPP_LOG_DEBUG("Plugin file modified, reloading...");

        std::string path = m_pluginInfo.path;
        unloadPlugin();

        if (loadPlugin(path)) {
            if (m_reloadCallback) {
                m_reloadCallback();
            }
            return true;
        } else {
            HOTPLUGPP_LOG_ERROR("Failed to reload plugin: " + path);
        }
    }

    return false;
}

IPlugin* PluginLoader::getPlugin() const {
    return m_pluginInfo.instance;
}

bool PluginLoader::isLoaded() const {
    return m_pluginInfo.isLoaded && m_pluginInfo.instance != nullptr;
}

std::string PluginLoader::getPluginPath() const {
    return m_pluginInfo.path;
}

void PluginLoader::setReloadCallback(std::function<void()> callback) {
    m_reloadCallback = std::move(callback);
}

std::chrono::system_clock::time_point
PluginLoader::getFileModificationTime(const std::string& path) {
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) == 0) {
        return std::chrono::system_clock::from_time_t(statbuf.st_mtime);
    }
    return std::chrono::system_clock::time_point();
}

LibraryHandle PluginLoader::loadLibrary(const std::string& path) {
#ifdef _WIN32
    return LoadLibraryA(path.c_str());
#else
    return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
}

void PluginLoader::unloadLibrary(LibraryHandle handle) {
    if (!handle)
        return;

#ifdef _WIN32
    FreeLibrary(handle);
#else
    dlclose(handle);
#endif
}

void* PluginLoader::getFunction(LibraryHandle handle, const std::string& name) {
    if (!handle)
        return nullptr;

#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(handle, name.c_str()));
#else
    return dlsym(handle, name.c_str());
#endif
}

std::string PluginLoader::getLastError() {
#ifdef _WIN32
    DWORD error = GetLastError();
    if (error == 0)
        return "No error";

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    if (size == 0 || messageBuffer == nullptr) {
        return "Unknown error (code: " + std::to_string(error) + ")";
    }

    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    return message;
#else
    const char* error = dlerror();
    return error ? std::string(error) : "No error";
#endif
}

} // namespace hotplugpp
