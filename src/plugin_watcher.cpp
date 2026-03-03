#include "plugin_watcher.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <condition_variable>
#include <exception>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

#if defined(HOTPLUGPP_HAS_EFSW)
#include <efsw/efsw.hpp>
#endif

#include <sys/stat.h>

namespace hotplugpp::detail {

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

// Returns nullopt when the file is inaccessible (does not exist, permission denied, etc.).
std::optional<std::chrono::system_clock::time_point>
getFileModificationTimeForWatch(const std::string& path) {
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) == 0) {
        return std::chrono::system_clock::from_time_t(statbuf.st_mtime);
    }
    return std::nullopt;
}

} // namespace

struct PluginWatcher::Impl {
    std::atomic<bool> stopRequested{false};
    std::atomic<bool> reloadPending{false};
    std::atomic<bool> watchActive{false};
    std::thread watcherThread;
    std::mutex mutex;
    std::condition_variable callbackIdle;
    // Always accessed under mutex; declared atomic for additional clarity and
    // to avoid any potential ordering issues on exotic platforms.
    std::atomic<std::size_t> callbacksInFlight{0};
    Clock::time_point lastEventTime{};
    std::string watchedPath;
    std::string watchedDirectory;
    std::string watchedFilename;

#if defined(HOTPLUGPP_HAS_EFSW)
    class ReloadListener final : public efsw::FileWatchListener {
      public:
        explicit ReloadListener(Impl& owner) : m_owner(owner) {}

        void handleFileAction(efsw::WatchID, const std::string& directory,
                              const std::string& filename, efsw::Action,
                              std::string oldFilename) override {
            CallbackActivityGuard callbackGuard(
                m_owner, normalizeWatchPathString(fs::path(directory)), filename, oldFilename);
            if (!callbackGuard) {
                return;
            }

            callbackGuard.notifyPluginChange();
        }

      private:
        class CallbackActivityGuard {
          public:
            CallbackActivityGuard(Impl& state, const std::string& directory,
                                  const std::string& filename, const std::string& oldFilename)
                : m_state(state) {
                std::lock_guard<std::mutex> lock(m_state.mutex);
                if (m_state.stopRequested.load(std::memory_order_acquire) ||
                    m_state.watchedFilename.empty() || m_state.watchedDirectory != directory) {
                    return;
                }

                const auto matches = [this](const std::string& candidate) {
                    if (candidate.empty()) {
                        return false;
                    }

                    return normalizeWatchPathString(fs::path(candidate).filename()) ==
                           m_state.watchedFilename;
                };

                if (!matches(filename) && !matches(oldFilename)) {
                    return;
                }

                ++m_state.callbacksInFlight;
                m_active = true;
            }

            ~CallbackActivityGuard() {
                if (!m_active) {
                    return;
                }

                std::lock_guard<std::mutex> lock(m_state.mutex);
                --m_state.callbacksInFlight;
                m_state.callbackIdle.notify_all();
            }

            explicit operator bool() const { return m_active; }

            void notifyPluginChange() {
                std::lock_guard<std::mutex> lock(m_state.mutex);
                m_state.lastEventTime = Clock::now();
                m_state.reloadPending.store(true, std::memory_order_release);
            }

          private:
            Impl& m_state;
            bool m_active = false;
        };

        Impl& m_owner;
    };

    std::unique_ptr<efsw::FileWatcher> fileWatcher;
    efsw::WatchID watchId = static_cast<efsw::WatchID>(-1);
    ReloadListener reloadListener;
#endif

    Impl()
#if defined(HOTPLUGPP_HAS_EFSW)
        : reloadListener(*this)
#endif
    {
    }

    void setWatchedTarget(std::string path, std::string directory, std::string filename) {
        std::lock_guard<std::mutex> lock(mutex);
        watchedPath = std::move(path);
        watchedDirectory = std::move(directory);
        watchedFilename = std::move(filename);
    }
};

PluginWatcher::PluginWatcher() : m_impl(std::make_unique<Impl>()) {}

PluginWatcher::~PluginWatcher() {
    stop();
}

void PluginWatcher::stop() {
    m_impl->stopRequested.store(true, std::memory_order_release);

#if defined(HOTPLUGPP_HAS_EFSW)
    if (m_impl->fileWatcher && m_impl->watchId >= 0) {
        m_impl->fileWatcher->removeWatch(m_impl->watchId);
    }
#endif

    if (m_impl->watcherThread.joinable()) {
        m_impl->watcherThread.join();
    }

    m_impl->reloadPending.store(false, std::memory_order_release);
    m_impl->watchActive.store(false, std::memory_order_release);

    {
        std::unique_lock<std::mutex> lock(m_impl->mutex);
        m_impl->callbackIdle.wait(lock, [this]() { return m_impl->callbacksInFlight == 0; });
        m_impl->lastEventTime = Clock::time_point{};
        m_impl->watchedPath.clear();
        m_impl->watchedDirectory.clear();
        m_impl->watchedFilename.clear();
    }

#if defined(HOTPLUGPP_HAS_EFSW)
    m_impl->fileWatcher.reset();
    m_impl->watchId = static_cast<efsw::WatchID>(-1);
#endif
}

bool PluginWatcher::start(const std::string& path) {
    stop();

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

    m_impl->setWatchedTarget(watchedPath, watchedDirectory, watchedFilename);
    m_impl->stopRequested.store(false, std::memory_order_release);

#if defined(HOTPLUGPP_HAS_EFSW)
    try {
        m_impl->fileWatcher = std::make_unique<efsw::FileWatcher>();
        m_impl->watchId = m_impl->fileWatcher->addWatch(watchedDirectory, &m_impl->reloadListener,
                                                        false);
        if (m_impl->watchId >= 0) {
            m_impl->fileWatcher->watch();
            m_impl->watchActive.store(true, std::memory_order_release);
            return true;
        }
    } catch (const std::exception& ex) {
        std::cerr << "efsw watcher setup failed for " << path << ": " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "efsw watcher setup failed for " << path << "." << std::endl;
    }

    m_impl->fileWatcher.reset();
    m_impl->watchId = static_cast<efsw::WatchID>(-1);
    std::cerr << "efsw watcher unavailable for " << path
              << "; falling back to the built-in polling watcher." << std::endl;
#endif

    try {
        m_impl->watcherThread = std::thread([impl = m_impl.get(), watchedPath]() {
            try {
                auto lastObserved = getFileModificationTimeForWatch(watchedPath);
                while (!impl->stopRequested.load(std::memory_order_acquire)) {
                    const auto currentObserved = getFileModificationTimeForWatch(watchedPath);
                    if (currentObserved && lastObserved && *currentObserved > *lastObserved) {
                        lastObserved = currentObserved;
                        std::lock_guard<std::mutex> lock(impl->mutex);
                        impl->lastEventTime = Clock::now();
                        impl->reloadPending.store(true, std::memory_order_release);
                    } else if (currentObserved) {
                        // File is accessible but mtime did not advance (equal or rare
                        // regression). Keep the reference in sync so a genuine future
                        // change is still detected correctly.
                        lastObserved = currentObserved;
                    }
                    // If currentObserved is nullopt the file is temporarily inaccessible;
                    // retain lastObserved so we detect the change once it reappears.

                    std::this_thread::sleep_for(kWatchPollInterval);
                }
            } catch (const std::exception& ex) {
                std::cerr << "[hotplugpp] Polling watcher thread exception: " << ex.what()
                          << std::endl;
            } catch (...) {
                std::cerr << "[hotplugpp] Polling watcher thread: unknown exception." << std::endl;
            }
        });
    } catch (const std::exception& ex) {
        stop();
        std::cerr << "Polling watcher setup failed for " << path << ": " << ex.what() << std::endl;
        return false;
    } catch (...) {
        stop();
        std::cerr << "Polling watcher setup failed for " << path << "." << std::endl;
        return false;
    }

    m_impl->watchActive.store(true, std::memory_order_release);
    return true;
}

bool PluginWatcher::consumeReloadSignal(
    const std::chrono::system_clock::time_point& currentModTime,
    const std::chrono::system_clock::time_point& lastKnownModTime) {
    const bool fileChanged = currentModTime > lastKnownModTime;

    // When the watcher is inactive or has not queued a reload event, fall back
    // to a direct mtime comparison so the caller still detects changes without
    // an active watcher (e.g. when the plugin path was rejected at start()).
    if (!m_impl->watchActive.load(std::memory_order_acquire) ||
        !m_impl->reloadPending.load(std::memory_order_acquire)) {
        return fileChanged;
    }

    Clock::time_point lastEventTime;
    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        lastEventTime = m_impl->lastEventTime;
    }

    if (lastEventTime != Clock::time_point{} &&
        (Clock::now() - lastEventTime) < kReloadDebounceInterval) {
        return false;
    }

    m_impl->reloadPending.store(false, std::memory_order_release);

    // Trust the watcher event: reload if the file exists (non-default mtime),
    // even when the mtime comparison alone would not indicate a change.
    const auto kNoModTime = std::chrono::system_clock::time_point{};
    return fileChanged || currentModTime != kNoModTime;
}

} // namespace hotplugpp::detail
