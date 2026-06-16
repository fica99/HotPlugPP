#include "hotplugpp/plugin_loader.hpp"

#include "plugin_watcher.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <system_error>
#include <thread>
#include <utility>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

#include <sys/stat.h>

namespace hotplugpp {

namespace {

namespace fs = std::filesystem;

// Filename prefix for the loader's co-located shadow copies. Kept in sync with the
// test suite (tests/plugin_loader_tests.cpp).
constexpr const char* kShadowPrefix = ".hotplugpp-shadow-";
// A rebuild's linker may briefly hold the output file when the watcher fires;
// retry the copy a few times before giving up.
constexpr int kCopyRetries = 20;
constexpr auto kCopyRetryDelay = std::chrono::milliseconds(25);

unsigned long currentProcessId() {
#ifdef _WIN32
    return static_cast<unsigned long>(GetCurrentProcessId());
#else
    return static_cast<unsigned long>(getpid());
#endif
}

// Copies originalPath to a uniquely named sibling file (same directory) and returns
// the absolute shadow path, or nullopt if no copy could be made. Loading this copy
// instead of the original keeps the build-output file unlocked so a rebuild can
// overwrite it (fixes the Windows DLL lock that broke hot-reload).
std::optional<std::string> makeShadowCopy(const std::string& originalPath) {
    if (originalPath.empty()) {
        return std::nullopt;
    }

    std::error_code ec;
    fs::path original = fs::absolute(originalPath, ec);
    if (ec) {
        original = fs::path(originalPath);
        ec.clear();
    }

    fs::path directory = original.has_parent_path() ? original.parent_path() : fs::current_path(ec);
    if (ec || directory.empty()) {
        return std::nullopt;
    }
    // Normalize once so the returned shadow path (later used for removal) is absolute.
    if (fs::path absoluteDir = fs::absolute(directory, ec); !ec) {
        directory = std::move(absoluteDir);
    }

    // Process-unique, monotonic name: the atomic counter never repeats within a run,
    // and copy_file(overwrite_existing) tolerates a stale leftover from a previous
    // run that happened to reuse this pid.
    static std::atomic<unsigned long long> counter{0};
    const std::string name = std::string(kShadowPrefix) + original.stem().string() + "-" +
                             std::to_string(currentProcessId()) + "-" +
                             std::to_string(counter.fetch_add(1)) + original.extension().string();
    const fs::path shadow = directory / name;

    for (int attempt = 0; attempt < kCopyRetries; ++attempt) {
        ec.clear();
        fs::copy_file(original, shadow, fs::copy_options::overwrite_existing, ec);
        if (!ec) {
            return shadow.string();
        }
        // A missing source will not reappear; the retry budget exists only for a
        // rebuild's transient write lock, so do not burn it here.
        if (ec == std::errc::no_such_file_or_directory) {
            break;
        }
        std::this_thread::sleep_for(kCopyRetryDelay);
    }

    // Remove any partial candidate so a failed copy is not left behind.
    std::error_code removeEc;
    fs::remove(shadow, removeEc);
    return std::nullopt;
}

class DynamicLibrary {
  public:
    DynamicLibrary() = default;
    ~DynamicLibrary() { reset(); }

    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;

    DynamicLibrary(DynamicLibrary&& other) noexcept
        : m_handle(std::exchange(other.m_handle, nullptr)),
          m_shadowPath(std::move(other.m_shadowPath)) {
        other.m_shadowPath.clear();
    }

    DynamicLibrary& operator=(DynamicLibrary&& other) noexcept {
        if (this != &other) {
            reset();
            m_handle = std::exchange(other.m_handle, nullptr);
            m_shadowPath = std::move(other.m_shadowPath);
            other.m_shadowPath.clear();
        }
        return *this;
    }

    [[nodiscard]] bool load(const std::string& path) {
        reset();

        // Prefer loading a co-located shadow copy so the original stays unlocked.
        if (std::optional<std::string> shadow = makeShadowCopy(path)) {
            if (loadNative(*shadow)) {
                m_shadowPath = std::move(*shadow);
                return true;
            }
            // The copy is the same bytes as the original, so do not retry the
            // original. Remove the copy, preserving the load error (on Windows the
            // removal's DeleteFileW would otherwise overwrite GetLastError).
#ifdef _WIN32
            const DWORD loadError = ::GetLastError();
#endif
            std::error_code ec;
            fs::remove(*shadow, ec);
#ifdef _WIN32
            ::SetLastError(loadError);
#endif
            return false;
        }

        // No shadow copy could be made (e.g. a read-only directory): load the
        // original directly, preserving the historical behavior. On Windows the
        // original is then locked, so hot-reload may not work for it.
        return loadNative(path);
    }

    void reset() noexcept {
        if (m_handle) {
#ifdef _WIN32
            FreeLibrary(m_handle);
#else
            dlclose(m_handle);
#endif
            m_handle = nullptr;
        }

        if (!m_shadowPath.empty()) {
            std::error_code ec;
            fs::remove(m_shadowPath, ec); // best-effort
            m_shadowPath.clear();
        }
    }

    [[nodiscard]] explicit operator bool() const noexcept { return m_handle != nullptr; }

    [[nodiscard]] void* symbol(const std::string& name) const {
        if (!m_handle) {
            return nullptr;
        }

#ifdef _WIN32
        return reinterpret_cast<void*>(GetProcAddress(m_handle, name.c_str()));
#else
        return dlsym(m_handle, name.c_str());
#endif
    }

  private:
    [[nodiscard]] bool loadNative(const std::string& path) {
#ifdef _WIN32
        const fs::path nativePath(path);
        m_handle = LoadLibraryW(nativePath.c_str());
#else
        dlerror();
        m_handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
        return m_handle != nullptr;
    }

#ifdef _WIN32
    HMODULE m_handle = nullptr;
#else
    void* m_handle = nullptr;
#endif
    std::string m_shadowPath;
};

std::string getLastDynamicLibraryError() {
#ifdef _WIN32
    const DWORD error = GetLastError();
    if (error == 0) {
        return "No error";
    }

    LPSTR messageBuffer = nullptr;
    const size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                           FORMAT_MESSAGE_IGNORE_INSERTS,
                                       nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                       reinterpret_cast<LPSTR>(&messageBuffer), 0, nullptr);

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

// Returns nullopt when the file is inaccessible (does not exist, permission denied, etc.).
std::optional<std::chrono::system_clock::time_point>
getFileModificationTimeForWatch(const std::string& path) {
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) == 0) {
        return std::chrono::system_clock::from_time_t(statbuf.st_mtime);
    }
    return std::nullopt;
}

struct LoadedPlugin {
    std::string path;
    DynamicLibrary library;
    IPlugin* instance = nullptr;
    CreatePluginFunc createFunc = nullptr;
    DestroyPluginFunc destroyFunc = nullptr;
    std::chrono::system_clock::time_point lastModified;
    bool isLoaded = false;
};

} // namespace

struct PluginLoader::Impl {
    Impl() : watcher(std::make_unique<detail::PluginWatcher>()) {}

    ~Impl() { unloadPlugin(); }

    bool loadPlugin(const std::string& path) {
        if (isLoaded()) {
            unloadPlugin();
        }

        DynamicLibrary library;
        if (!library.load(path)) {
            std::cerr << "Failed to load library: " << path << std::endl;
            std::cerr << "Error: " << getLastDynamicLibraryError() << std::endl;
            return false;
        }

        void* createSymbol = library.symbol("createPlugin");
        if (!createSymbol) {
            std::cerr << "Failed to find createPlugin in: " << path << std::endl;
            std::cerr << "Error: " << getLastDynamicLibraryError() << std::endl;
            return false;
        }

        void* destroySymbol = library.symbol("destroyPlugin");
        if (!destroySymbol) {
            std::cerr << "Failed to find destroyPlugin in: " << path << std::endl;
            std::cerr << "Error: " << getLastDynamicLibraryError() << std::endl;
            return false;
        }

        CreatePluginFunc createFunc = reinterpret_cast<CreatePluginFunc>(createSymbol);
        DestroyPluginFunc destroyFunc = reinterpret_cast<DestroyPluginFunc>(destroySymbol);

        IPlugin* plugin = createFunc();
        if (!plugin) {
            std::cerr << "Failed to create plugin instance from: " << path << std::endl;
            return false;
        }

        if (!plugin->onLoad()) {
            std::cerr << "Plugin initialization failed: " << path << std::endl;
            destroyFunc(plugin);
            return false;
        }

        pluginInfo.path = path;
        pluginInfo.library = std::move(library);
        pluginInfo.instance = plugin;
        pluginInfo.createFunc = createFunc;
        pluginInfo.destroyFunc = destroyFunc;
        pluginInfo.lastModified = getFileModificationTime(path);
        pluginInfo.isLoaded = true;

        if (!watcher->start(path)) {
            std::cerr << "Continuing without file watcher for: " << path << std::endl;
        }

        std::cout << "Plugin loaded successfully: " << plugin->getName() << " v"
                  << plugin->getVersion().toString() << std::endl;

        return true;
    }

    void unloadPlugin() {
        if (!isLoaded()) {
            return;
        }

        watcher->stop();

        if (pluginInfo.instance) {
            pluginInfo.instance->onUnload();
            if (pluginInfo.destroyFunc) {
                pluginInfo.destroyFunc(pluginInfo.instance);
            }
            pluginInfo.instance = nullptr;
        }

        pluginInfo.library.reset();
        pluginInfo.isLoaded = false;
        pluginInfo.createFunc = nullptr;
        pluginInfo.destroyFunc = nullptr;
    }

    PluginLoader::ReloadResult checkAndReload() {
        if (!isLoaded()) {
            return PluginLoader::ReloadResult::NoChange;
        }

        const auto currentModTime = getFileModificationTime(pluginInfo.path);
        if (watcher->consumeReloadSignal(currentModTime, pluginInfo.lastModified)) {
            std::cout << "Plugin file modified, reloading..." << std::endl;

            const std::string path = pluginInfo.path;
            unloadPlugin();

            if (loadPlugin(path)) {
                if (reloadCallback) {
                    reloadCallback();
                }
                return PluginLoader::ReloadResult::Reloaded;
            }

            std::cerr << "Failed to reload plugin: " << path << std::endl;
            return PluginLoader::ReloadResult::ReloadFailed;
        }

        return PluginLoader::ReloadResult::NoChange;
    }

    [[nodiscard]] IPlugin* getPlugin() const noexcept { return pluginInfo.instance; }

    [[nodiscard]] bool isLoaded() const noexcept {
        return pluginInfo.isLoaded && pluginInfo.instance != nullptr &&
               static_cast<bool>(pluginInfo.library);
    }

    [[nodiscard]] std::string getPluginPath() const { return pluginInfo.path; }

    void setReloadCallback(std::function<void()> callback) { reloadCallback = std::move(callback); }

    [[nodiscard]] std::chrono::system_clock::time_point
    getFileModificationTime(const std::string& path) const {
        return getFileModificationTimeForWatch(path).value_or(
            std::chrono::system_clock::time_point{});
    }

    LoadedPlugin pluginInfo;
    std::function<void()> reloadCallback;
    std::unique_ptr<detail::PluginWatcher> watcher;
};

PluginLoader::PluginLoader() : m_impl(std::make_unique<Impl>()) {}

PluginLoader::~PluginLoader() = default;

bool PluginLoader::loadPlugin(const std::string& path) {
    if (!m_impl) {
        m_impl = std::make_unique<Impl>();
    }
    return m_impl->loadPlugin(path);
}

void PluginLoader::unloadPlugin() {
    if (m_impl) {
        m_impl->unloadPlugin();
    }
}

PluginLoader::ReloadResult PluginLoader::checkAndReload() {
    if (!m_impl) {
        return ReloadResult::NoChange;
    }
    return m_impl->checkAndReload();
}

IPlugin* PluginLoader::getPlugin() const {
    return m_impl ? m_impl->getPlugin() : nullptr;
}

bool PluginLoader::isLoaded() const {
    return m_impl != nullptr && m_impl->isLoaded();
}

std::string PluginLoader::getPluginPath() const {
    return m_impl ? m_impl->getPluginPath() : std::string{};
}

void PluginLoader::setReloadCallback(std::function<void()> callback) {
    if (!m_impl) {
        m_impl = std::make_unique<Impl>();
    }
    m_impl->setReloadCallback(std::move(callback));
}

} // namespace hotplugpp
