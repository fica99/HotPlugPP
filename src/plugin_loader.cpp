#include "hotplugpp/plugin_loader.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <condition_variable>
#include <exception>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#if defined(HOTPLUGPP_HAS_EFSW)
#include <efsw/efsw.hpp>
#endif

#include <sys/stat.h>

namespace hotplugpp {

namespace {

using Clock = std::chrono::steady_clock;
namespace fs = std::filesystem;

constexpr auto kWatchPollInterval = std::chrono::milliseconds(100);
constexpr auto kReloadDebounceInterval = std::chrono::milliseconds(250);

std::string toLowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

bool isDynamicLibraryPath(const fs::path& path) {
    const std::string extension = toLowerCopy(path.extension().string());
    return extension == ".dll" || extension == ".so" || extension == ".dylib";
}

std::string normalizeWatchPathString(const fs::path& path) {
    std::string normalized = path.lexically_normal().string();
#ifdef _WIN32
    normalized = toLowerCopy(std::move(normalized));
#endif
    return normalized;
}

struct WatcherState;

#if defined(HOTPLUGPP_HAS_EFSW)
class ReloadListener;
#endif

struct WatcherState {
    std::atomic<bool> stopRequested{false};
    std::atomic<bool> reloadPending{false};
    bool watchActive = false;
    std::thread watcherThread;
    mutable std::mutex mutex;
    std::condition_variable callbackIdle;
    std::size_t callbacksInFlight = 0;
    Clock::time_point lastEventTime{};
    std::string watchedPath;
    std::string watchedDirectory;
    std::string watchedFilename;

#if defined(HOTPLUGPP_HAS_EFSW)
    std::unique_ptr<efsw::FileWatcher> fileWatcher;
    efsw::WatchID watchId = static_cast<efsw::WatchID>(-1);
#endif

    void setWatchedTarget(std::string path, std::string directory, std::string filename) {
        std::lock_guard<std::mutex> lock(mutex);
        watchedPath = std::move(path);
        watchedDirectory = std::move(directory);
        watchedFilename = std::move(filename);
    }
};

std::mutex g_watcherStatesMutex;
std::unordered_map<const PluginLoader*, std::unique_ptr<WatcherState>> g_watcherStates;

#if defined(HOTPLUGPP_HAS_EFSW)
std::mutex g_activeWatchesMutex;
std::unordered_set<WatcherState*> g_activeWatches;

class CallbackActivityGuard {
  public:
    CallbackActivityGuard(WatcherState& state, const std::string& directory,
                          const std::string& filename, const std::string& oldFilename) {
        std::lock_guard<std::mutex> lock(state.mutex);
        if (state.stopRequested.load(std::memory_order_acquire) || state.watchedFilename.empty() ||
            state.watchedDirectory != directory) {
            return;
        }

        const auto matches = [&state](const std::string& candidate) {
            if (candidate.empty()) {
                return false;
            }

            return normalizeWatchPathString(fs::path(candidate).filename()) ==
                   state.watchedFilename;
        };

        if (!matches(filename) && !matches(oldFilename)) {
            return;
        }

        ++state.callbacksInFlight;
        m_state = &state;
    }

    ~CallbackActivityGuard() {
        if (!m_state) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_state->mutex);
        --m_state->callbacksInFlight;
        m_state->callbackIdle.notify_all();
    }

    explicit operator bool() const { return m_state != nullptr; }

    WatcherState& state() const { return *m_state; }

  private:
    WatcherState* m_state = nullptr;
};
#endif

std::chrono::system_clock::time_point getFileModificationTimeForWatch(const std::string& path) {
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) == 0) {
        return std::chrono::system_clock::from_time_t(statbuf.st_mtime);
    }

    return std::chrono::system_clock::time_point();
}

WatcherState& getWatcherState(const PluginLoader* loader) {
    std::lock_guard<std::mutex> lock(g_watcherStatesMutex);
    auto [it, inserted] = g_watcherStates.emplace(loader, std::make_unique<WatcherState>());
    (void)inserted;
    return *it->second;
}

WatcherState* findWatcherState(const PluginLoader* loader) {
    std::lock_guard<std::mutex> lock(g_watcherStatesMutex);
    const auto it = g_watcherStates.find(loader);
    if (it == g_watcherStates.end()) {
        return nullptr;
    }

    return it->second.get();
}

void notifyPluginChange(WatcherState& state) {
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.lastEventTime = Clock::now();
    }
    state.reloadPending.store(true, std::memory_order_release);
}

#if defined(HOTPLUGPP_HAS_EFSW)
class ReloadListener final : public efsw::FileWatchListener {
  public:
    void handleFileAction(efsw::WatchID, const std::string& directory, const std::string& filename,
                          efsw::Action action, std::string oldFilename) override {
        (void)action;
        const std::string normalizedDirectory = normalizeWatchPathString(fs::path(directory));

        std::lock_guard<std::mutex> registryLock(g_activeWatchesMutex);
        for (WatcherState* state : g_activeWatches) {
            CallbackActivityGuard callbackGuard(*state, normalizedDirectory, filename, oldFilename);
            if (!callbackGuard) {
                continue;
            }

            notifyPluginChange(callbackGuard.state());
        }
    }
};

ReloadListener& getReloadListener() {
    static ReloadListener listener;
    return listener;
}

void registerActiveWatch(WatcherState& state) {
    std::lock_guard<std::mutex> lock(g_activeWatchesMutex);
    g_activeWatches.insert(&state);
}

void unregisterActiveWatch(WatcherState& state) {
    std::lock_guard<std::mutex> lock(g_activeWatchesMutex);
    g_activeWatches.erase(&state);
}
#endif

void stopWatching(WatcherState& state) {
    state.stopRequested.store(true, std::memory_order_release);

#if defined(HOTPLUGPP_HAS_EFSW)
    if (state.watchId >= 0) {
        unregisterActiveWatch(state);
    }

    if (state.fileWatcher && state.watchId >= 0) {
        state.fileWatcher->removeWatch(state.watchId);
    }
#endif

    if (state.watcherThread.joinable()) {
        state.watcherThread.join();
    }

    state.reloadPending.store(false, std::memory_order_release);
    state.watchActive = false;

    {
        std::unique_lock<std::mutex> lock(state.mutex);
        state.callbackIdle.wait(lock, [&state]() { return state.callbacksInFlight == 0; });
        state.lastEventTime = Clock::time_point{};
        state.watchedPath.clear();
        state.watchedDirectory.clear();
        state.watchedFilename.clear();
    }

#if defined(HOTPLUGPP_HAS_EFSW)
    state.fileWatcher.reset();
    state.watchId = static_cast<efsw::WatchID>(-1);
#endif
}

bool startWatching(const PluginLoader* loader, const std::string& path) {
    WatcherState& state = getWatcherState(loader);
    stopWatching(state);

    fs::path pluginPath(path);
    if (pluginPath.empty() || !isDynamicLibraryPath(pluginPath)) {
        std::cerr << "Hot-reload watcher only supports plugin libraries: " << path << std::endl;
        return false;
    }

    std::error_code pathError;
    fs::path absolutePluginPath = fs::absolute(pluginPath, pathError);
    if (pathError) {
        absolutePluginPath = pluginPath.lexically_normal();
        pathError.clear();
    }

    fs::path pluginDirectory = absolutePluginPath.has_parent_path()
                                   ? absolutePluginPath.parent_path()
                                   : fs::current_path(pathError);
    const bool directoryExists = !pluginDirectory.empty() && fs::exists(pluginDirectory, pathError);
    if (pathError || !directoryExists) {
        std::cerr << "Hot-reload watcher could not monitor path: " << path << std::endl;
        return false;
    }

    const std::string watchedPath = normalizeWatchPathString(absolutePluginPath);
    const std::string watchedDirectory = normalizeWatchPathString(pluginDirectory);
    const std::string watchedFilename = normalizeWatchPathString(absolutePluginPath.filename());

    state.setWatchedTarget(watchedPath, watchedDirectory, watchedFilename);
    state.stopRequested.store(false, std::memory_order_release);

#if defined(HOTPLUGPP_HAS_EFSW)
    bool registeredWithEfsw = false;
    try {
        state.fileWatcher = std::make_unique<efsw::FileWatcher>();
        state.watchId = state.fileWatcher->addWatch(watchedDirectory, &getReloadListener(), false);

        if (state.watchId >= 0) {
            registerActiveWatch(state);
            registeredWithEfsw = true;
            state.fileWatcher->watch();
            state.watchActive = true;
            return true;
        }
    } catch (const std::exception& ex) {
        if (registeredWithEfsw) {
            unregisterActiveWatch(state);
        }
        std::cerr << "efsw watcher setup failed for " << path << ": " << ex.what() << std::endl;
    } catch (...) {
        if (registeredWithEfsw) {
            unregisterActiveWatch(state);
        }
        std::cerr << "efsw watcher setup failed for " << path << "." << std::endl;
    }

    state.fileWatcher.reset();
    state.watchId = static_cast<efsw::WatchID>(-1);
    std::cerr << "efsw watcher unavailable for " << path
              << "; falling back to the built-in polling watcher." << std::endl;
#endif

    try {
        state.watcherThread = std::thread([&state, watchedPath]() {
            auto lastObserved = getFileModificationTimeForWatch(watchedPath);

            while (!state.stopRequested.load(std::memory_order_acquire)) {
                const auto currentObserved = getFileModificationTimeForWatch(watchedPath);
                if (currentObserved > lastObserved) {
                    lastObserved = currentObserved;
                    notifyPluginChange(state);
                } else if (currentObserved != std::chrono::system_clock::time_point()) {
                    lastObserved = currentObserved;
                }

                std::this_thread::sleep_for(kWatchPollInterval);
            }
        });
    } catch (const std::exception& ex) {
        stopWatching(state);
        std::cerr << "Polling watcher setup failed for " << path << ": " << ex.what() << std::endl;
        return false;
    } catch (...) {
        stopWatching(state);
        std::cerr << "Polling watcher setup failed for " << path << "." << std::endl;
        return false;
    }
    state.watchActive = true;

    return true;
}

void destroyWatcherState(const PluginLoader* loader) {
    std::unique_ptr<WatcherState> state;
    {
        std::lock_guard<std::mutex> lock(g_watcherStatesMutex);
        auto it = g_watcherStates.find(loader);
        if (it == g_watcherStates.end()) {
            return;
        }

        state = std::move(it->second);
        g_watcherStates.erase(it);
    }

    stopWatching(*state);
}

bool consumeReloadSignal(const PluginLoader* loader,
                         const std::chrono::system_clock::time_point& currentModTime,
                         const std::chrono::system_clock::time_point& lastKnownModTime) {
    const bool fileChanged = currentModTime > lastKnownModTime;
    WatcherState* state = findWatcherState(loader);
    if (!state) {
        return fileChanged;
    }

    if (!state->watchActive || !state->reloadPending.load(std::memory_order_acquire)) {
        return fileChanged;
    }

    Clock::time_point lastEventTime;
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        lastEventTime = state->lastEventTime;
    }

    if (lastEventTime != Clock::time_point{} &&
        (Clock::now() - lastEventTime) < kReloadDebounceInterval) {
        return false;
    }

    state->reloadPending.store(false, std::memory_order_release);
    return fileChanged || currentModTime != std::chrono::system_clock::time_point();
}

} // namespace

PluginLoader::PluginLoader() {
    getWatcherState(this);
}

PluginLoader::~PluginLoader() {
    unloadPlugin();
    destroyWatcherState(this);
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

    if (!startWatching(this, path)) {
        std::cerr << "Continuing without file watcher for: " << path << std::endl;
    }

    std::cout << "Plugin loaded successfully: " << plugin->getName() << " v"
              << plugin->getVersion().toString() << std::endl;

    return true;
}

void PluginLoader::unloadPlugin() {
    if (!isLoaded()) {
        return;
    }

    if (WatcherState* state = findWatcherState(this)) {
        stopWatching(*state);
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
    m_pluginInfo.instance = nullptr;
}

bool PluginLoader::checkAndReload() {
    if (!isLoaded()) {
        return false;
    }

    auto currentModTime = getFileModificationTime(m_pluginInfo.path);
    if (consumeReloadSignal(this, currentModTime, m_pluginInfo.lastModified)) {
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

std::chrono::system_clock::time_point
PluginLoader::getFileModificationTime(const std::string& path) {
    return getFileModificationTimeForWatch(path);
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
