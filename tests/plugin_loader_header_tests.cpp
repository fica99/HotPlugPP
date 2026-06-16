#include "hotplugpp/plugin_loader.hpp"

#ifdef _WIN32
#ifdef min
#error "plugin_loader.hpp must not expose the Windows min macro"
#endif

#ifdef max
#error "plugin_loader.hpp must not expose the Windows max macro"
#endif
#endif

#include <gtest/gtest.h>

namespace hotplugpp {
namespace tests {

TEST(PluginLoaderHeaderTest, PublicHeaderDoesNotRequirePlatformHeaders) {
    PluginLoader loader;

    EXPECT_FALSE(loader.isLoaded());
}

} // namespace tests
} // namespace hotplugpp
