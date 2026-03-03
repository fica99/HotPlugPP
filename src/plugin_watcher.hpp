#pragma once

#include <chrono>
#include <memory>
#include <string>

namespace hotplugpp::detail {

class PluginWatcher {
  public:
    PluginWatcher();
    ~PluginWatcher();

    PluginWatcher(const PluginWatcher&) = delete;
    PluginWatcher& operator=(const PluginWatcher&) = delete;

    bool start(const std::string& path);
    void stop();

    bool consumeReloadSignal(const std::chrono::system_clock::time_point& currentModTime,
                             const std::chrono::system_clock::time_point& lastKnownModTime);

  private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace hotplugpp::detail
