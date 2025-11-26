#include "hotplugpp/plugin_loader.hpp"

#include "hotplugpp/file_watcher.hpp"

#include <iostream>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <sys/stat.h>

namespace hotplugpp {

PluginLoader::PluginLoader() : m_fileWatcher(std::make_unique<FileWatcher>()) {}

PluginLoader::~PluginLoader() {
    enableAutoReload(false);
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
        std::cerr << "Failed to load library: " << path << std::endl;
        std::cerr << "Error: " << getLastError() << std::endl;
        return false;
    }

    // Get the factory functions
    CreatePluginFunc createFunc = reinterpret_cast<CreatePluginFunc>(
        getFunction(handle, "createPlugin"));
    DestroyPluginFunc destroyFunc = reinterpret_cast<DestroyPluginFunc>(
        getFunction(handle, "destroyPlugin"));

    if (!createFunc || !destroyFunc) {
        std::cerr << "Failed to find plugin factory functions in: " << path << std::endl;
        std::cerr << "Error: " << getLastError() << std::endl;
        unloadLibrary(handle);
        return false;
    }

    // Create plugin instance
    IPlugin* plugin = createFunc();
    if (!plugin) {
        std::cerr << "Failed to create plugin instance from: " << path << std::endl;
        unloadLibrary(handle);
        return false;
    }

    // Initialize plugin
    if (!plugin->onLoad()) {
        std::cerr << "Plugin initialization failed: " << path << std::endl;
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

    std::cout << "Plugin loaded successfully: " << plugin->getName() << " v"
              << plugin->getVersion().toString() << std::endl;

    // Setup file watching if auto-reload is enabled
    if (m_autoReloadEnabled.load()) {
        setupFileWatch();
    }

    return true;
}

void PluginLoader::unloadPlugin() {
    if (!isLoaded()) {
        return;
    }

    // Remove file watch first
    removeFileWatch();

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
    // First check for pending async reload
    if (processPendingReload()) {
        return true;
    }

    if (!isLoaded()) {
        return false;
    }

    auto currentModTime = getFileModificationTime(m_pluginInfo.path);

    // Check if file has been modified
    if (currentModTime > m_pluginInfo.lastModified) {
        std::cout << "Plugin file modified, reloading..." << std::endl;

        std::string path = m_pluginInfo.path;
        unloadPlugin();

        if (loadPlugin(path)) {
            if (m_reloadCallback) {
                m_reloadCallback();
            }
            return true;
        } else {
            std::cerr << "Failed to reload plugin: " << path << std::endl;
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

void PluginLoader::enableAutoReload(bool enable) {
    bool wasEnabled = m_autoReloadEnabled.exchange(enable);

    if (enable && !wasEnabled) {
        // Just enabled, start file watcher and setup watch
        m_fileWatcher->start();
        if (isLoaded()) {
            setupFileWatch();
        }
    } else if (!enable && wasEnabled) {
        // Just disabled, remove watch and stop watcher
        removeFileWatch();
        m_fileWatcher->stop();
    }
}

bool PluginLoader::isAutoReloadEnabled() const {
    return m_autoReloadEnabled.load();
}

void PluginLoader::setupFileWatch() {
    if (!isLoaded() || !m_autoReloadEnabled.load()) {
        return;
    }

    m_fileWatcher->watchFile(m_pluginInfo.path,
                             [this](const std::string& filePath) { onFileChanged(filePath); });
}

void PluginLoader::removeFileWatch() {
    if (!m_pluginInfo.path.empty()) {
        m_fileWatcher->unwatchFile(m_pluginInfo.path);
    }
}

void PluginLoader::onFileChanged(const std::string& /*filePath*/) {
    // Mark that we need to reload
    // The actual reload will happen in the main thread when checkAndReload or
    // processPendingReload is called
    m_pendingReload.store(true);
    std::cout << "File change detected, reload pending..." << std::endl;
}

bool PluginLoader::processPendingReload() {
    if (!m_pendingReload.exchange(false)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_reloadMutex);

    if (!isLoaded()) {
        return false;
    }

    std::cout << "Processing pending reload..." << std::endl;

    std::string path = m_pluginInfo.path;
    unloadPlugin();

    if (loadPlugin(path)) {
        if (m_reloadCallback) {
            m_reloadCallback();
        }
        return true;
    } else {
        std::cerr << "Failed to reload plugin: " << path << std::endl;
    }

    return false;
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
