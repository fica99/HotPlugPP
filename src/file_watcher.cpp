#include "hotplugpp/file_watcher.hpp"

#include <iostream>

#include <efsw/efsw.hpp>

namespace hotplugpp {

/**
 * @brief Implementation of efsw::FileWatchListener that delegates to FileWatcher
 */
class FileWatcher::FileWatchListenerImpl : public efsw::FileWatchListener {
  public:
    explicit FileWatchListenerImpl(FileWatcher& owner) : m_owner(owner) {}

    void handleFileAction(efsw::WatchID /*watchid*/, const std::string& dir,
                          const std::string& filename, efsw::Action action,
                          std::string /*oldFilename*/) override {
        // We only care about modifications
        if (action == efsw::Actions::Modified) {
            m_owner.onFileChanged(dir, filename);
        }
    }

  private:
    FileWatcher& m_owner;
};

FileWatcher::FileWatcher()
    : m_watcher(std::make_unique<efsw::FileWatcher>()),
      m_listener(std::make_unique<FileWatchListenerImpl>(*this)) {}

FileWatcher::~FileWatcher() {
    stop();
}

bool FileWatcher::watchFile(const std::string& filePath, FileChangeCallback callback) {
    if (filePath.empty() || !callback) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    std::string directory = getDirectory(filePath);
    std::string filename = getFilename(filePath);

    if (directory.empty() || filename.empty()) {
        std::cerr << "Invalid file path: " << filePath << std::endl;
        return false;
    }

    // Check if we're already watching this directory
    auto dirIt = m_directoryWatches.find(directory);
    if (dirIt == m_directoryWatches.end()) {
        // Add watch for this directory (non-recursive)
        efsw::WatchID watchId = m_watcher->addWatch(directory, m_listener.get(), false);
        if (watchId < 0) {
            std::cerr << "Failed to watch directory: " << directory << std::endl;
            return false;
        }
        m_directoryWatches[directory] = watchId;
    }

    // Register the callback for this specific file
    m_fileCallbacks[filePath] = std::move(callback);

    return true;
}

void FileWatcher::unwatchFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto callbackIt = m_fileCallbacks.find(filePath);
    if (callbackIt == m_fileCallbacks.end()) {
        return;
    }

    m_fileCallbacks.erase(callbackIt);

    // Check if we need to remove the directory watch
    std::string directory = getDirectory(filePath);

    // Check if there are other files being watched in this directory
    bool hasOtherFiles = false;
    for (const auto& pair : m_fileCallbacks) {
        if (getDirectory(pair.first) == directory) {
            hasOtherFiles = true;
            break;
        }
    }

    if (!hasOtherFiles) {
        auto dirIt = m_directoryWatches.find(directory);
        if (dirIt != m_directoryWatches.end()) {
            m_watcher->removeWatch(dirIt->second);
            m_directoryWatches.erase(dirIt);
        }
    }
}

bool FileWatcher::isWatching(const std::string& filePath) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_fileCallbacks.find(filePath) != m_fileCallbacks.end();
}

void FileWatcher::start() {
    if (!m_running.exchange(true)) {
        m_watcher->watch();
    }
}

void FileWatcher::stop() {
    m_running.store(false);
    // Note: efsw doesn't have an explicit stop method, but the watcher
    // will stop when destroyed
}

bool FileWatcher::isRunning() const {
    return m_running.load();
}

std::string FileWatcher::getDirectory(const std::string& filePath) const {
    size_t lastSlash = filePath.find_last_of("/\\");
    if (lastSlash == std::string::npos) {
        return ".";
    }
    return filePath.substr(0, lastSlash);
}

std::string FileWatcher::getFilename(const std::string& filePath) const {
    size_t lastSlash = filePath.find_last_of("/\\");
    if (lastSlash == std::string::npos) {
        return filePath;
    }
    return filePath.substr(lastSlash + 1);
}

void FileWatcher::onFileChanged(const std::string& dir, const std::string& filename) {
    std::string fullPath;
    // Handle case where dir is empty or current directory
    if (dir.empty() || dir == "." || dir == "./") {
        fullPath = filename;
    } else {
        fullPath = dir;
        // Normalize path separator
        if (fullPath.back() != '/' && fullPath.back() != '\\') {
            fullPath += '/';
        }
        fullPath += filename;
    }

    FileChangeCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_fileCallbacks.find(fullPath);
        if (it != m_fileCallbacks.end()) {
            callback = it->second;
        }
    }

    if (callback) {
        // Call the callback outside of the lock to avoid potential deadlocks
        callback(fullPath);
    }
}

} // namespace hotplugpp
