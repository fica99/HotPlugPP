#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace efsw {
class FileWatcher;
class FileWatchListener;
typedef long WatchID;
} // namespace efsw

namespace hotplugpp {

/**
 * @brief Callback type for file change notifications
 * @param filePath Full path to the changed file
 */
using FileChangeCallback = std::function<void(const std::string& filePath)>;

/**
 * @brief Asynchronous file watcher using efsw library
 *
 * Monitors files for changes and invokes callbacks when modifications are detected.
 * Runs in a separate thread and handles change notifications asynchronously.
 */
class FileWatcher {
  public:
    FileWatcher();
    ~FileWatcher();

    // Disable copy
    FileWatcher(const FileWatcher&) = delete;
    FileWatcher& operator=(const FileWatcher&) = delete;

    /**
     * @brief Start watching a file for changes
     * @param filePath Full path to the file to watch
     * @param callback Function to call when the file changes
     * @return true if watching was set up successfully, false otherwise
     *
     * @note The callback is moved (not copied); after this call, the original
     *       callback is in an unspecified state.
     * @note If called multiple times for the same file, the previous callback
     *       will be replaced.
     * @note This method watches the file's parent directory internally, not the
     *       file directly.
     */
    bool watchFile(const std::string& filePath, FileChangeCallback callback);

    /**
     * @brief Stop watching a file
     * @param filePath Path of the file to stop watching
     */
    void unwatchFile(const std::string& filePath);

    /**
     * @brief Check if a file is being watched
     * @param filePath Path of the file to check
     * @return true if the file is being watched, false otherwise
     */
    bool isWatching(const std::string& filePath) const;

    /**
     * @brief Start the file watcher (begins asynchronous monitoring)
     */
    void start();

    /**
     * @brief Mark the file watcher as stopped
     *
     * This method updates the internal running flag. Note that the underlying
     * efsw monitoring continues until the FileWatcher object is destroyed.
     * Callbacks may still be invoked after this method returns if file changes
     * are already queued.
     */
    void stop();

    /**
     * @brief Check if the watcher is running
     * @return true if the watcher is running, false otherwise
     */
    bool isRunning() const;

  private:
    class FileWatchListenerImpl;

    std::unique_ptr<efsw::FileWatcher> m_watcher;
    std::unique_ptr<FileWatchListenerImpl> m_listener;

    // Maps directory path to watch ID
    std::unordered_map<std::string, efsw::WatchID> m_directoryWatches;

    // Maps file path to its callback
    std::unordered_map<std::string, FileChangeCallback> m_fileCallbacks;

    mutable std::mutex m_mutex;
    std::atomic<bool> m_running{false};

    /**
     * @brief Get the directory containing the file
     * @param filePath Path to the file
     * @return Directory path
     */
    std::string getDirectory(const std::string& filePath) const;

    /**
     * @brief Get the filename from a full path
     * @param filePath Full path
     * @return Filename only
     */
    std::string getFilename(const std::string& filePath) const;

    /**
     * @brief Normalize path separators to forward slashes
     * @param path Path to normalize
     * @return Normalized path
     */
    std::string normalizePath(const std::string& path) const;

    /**
     * @brief Handle a file change event from efsw
     * @param dir Directory where the change occurred
     * @param filename Name of the changed file
     */
    void onFileChanged(const std::string& dir, const std::string& filename);

    friend class FileWatchListenerImpl;
};

} // namespace hotplugpp
