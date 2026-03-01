#pragma once

#include <chrono>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

namespace efsw {

using WatchID = int;

enum Action { Add, Delete, Modified, Moved };

class FileWatchListener {
  public:
    virtual ~FileWatchListener() = default;

    virtual void handleFileAction(WatchID watchId, const std::string& dir,
                                  const std::string& filename, Action action,
                                  std::string oldFilename) = 0;
};

class FileWatcher {
  public:
    FileWatcher() = default;

    ~FileWatcher() { stop(); }

    WatchID addWatch(const std::string& directory, FileWatchListener* listener, bool recursive) {
        if (listener == nullptr) {
            return -1;
        }

        namespace fs = std::filesystem;
        std::error_code error;
        fs::path dirPath(directory);
        if (!fs::exists(dirPath, error) || error || !fs::is_directory(dirPath, error)) {
            return -1;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        const WatchID watchId = ++m_nextWatchId;
        m_watches.emplace(watchId, Watch{
                                       normalizeDirectory(dirPath),
                                       listener,
                                       recursive,
                                       snapshotDirectory(dirPath, recursive),
                                   });
        return watchId;
    }

    void removeWatch(WatchID watchId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_watches.erase(watchId);
        if (m_watches.empty()) {
            m_shouldRun = false;
        }
    }

    void watch() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_worker.joinable() || m_watches.empty()) {
            return;
        }

        m_shouldRun = true;
        m_worker = std::thread([this]() { run(); });
    }

  private:
    using Snapshot = std::unordered_map<std::string, std::filesystem::file_time_type>;

    struct Watch {
        std::string directory;
        FileWatchListener* listener = nullptr;
        bool recursive = false;
        Snapshot files;
    };

    static std::string normalizeDirectory(const std::filesystem::path& directory) {
        std::filesystem::path normalized = directory.lexically_normal();
        std::string text = normalized.string();
        if (!text.empty() && text.back() != '/' && text.back() != '\\') {
            text.push_back(std::filesystem::path::preferred_separator);
        }
        return text;
    }

    static Snapshot snapshotDirectory(const std::filesystem::path& directory, bool recursive) {
        Snapshot snapshot;
        std::error_code error;

        if (recursive) {
            for (std::filesystem::recursive_directory_iterator it(directory, error), end;
                 !error && it != end; it.increment(error)) {
                if (!it->is_regular_file(error) || error) {
                    continue;
                }
                snapshot[it->path().filename().string()] = it->last_write_time(error);
            }
        } else {
            for (std::filesystem::directory_iterator it(directory, error), end; !error && it != end;
                 it.increment(error)) {
                if (!it->is_regular_file(error) || error) {
                    continue;
                }
                snapshot[it->path().filename().string()] = it->last_write_time(error);
            }
        }

        return snapshot;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_shouldRun = false;
        }

        if (m_worker.joinable()) {
            m_worker.join();
        }
    }

    void run() {
        while (true) {
            std::unordered_map<WatchID, Watch> watches;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (!m_shouldRun) {
                    break;
                }
                watches = m_watches;
            }

            for (const auto& entry : watches) {
                pollWatch(entry.first, entry.second);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    void pollWatch(WatchID watchId, const Watch& watch) {
        namespace fs = std::filesystem;
        const fs::path directoryPath = watch.directory;
        Snapshot current = snapshotDirectory(directoryPath, watch.recursive);
        Snapshot previous;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_watches.find(watchId);
            if (it == m_watches.end()) {
                return;
            }
            previous = it->second.files;
            it->second.files = current;
        }

        for (const auto& currentFile : current) {
            const auto previousIt = previous.find(currentFile.first);
            if (previousIt == previous.end()) {
                watch.listener->handleFileAction(watchId, watch.directory, currentFile.first, Add,
                                                 "");
                continue;
            }

            if (currentFile.second != previousIt->second) {
                watch.listener->handleFileAction(watchId, watch.directory, currentFile.first,
                                                 Modified, "");
            }
        }

        for (const auto& previousFile : previous) {
            if (current.find(previousFile.first) == current.end()) {
                watch.listener->handleFileAction(watchId, watch.directory, previousFile.first,
                                                 Delete, "");
            }
        }
    }

    std::mutex m_mutex;
    std::unordered_map<WatchID, Watch> m_watches;
    std::thread m_worker;
    WatchID m_nextWatchId = 0;
    bool m_shouldRun = false;
};

} // namespace efsw
