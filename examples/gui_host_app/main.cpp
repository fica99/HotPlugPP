#include "hotplugpp/plugin_loader.hpp"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>

#include <GLFW/glfw3.h>

namespace {

constexpr std::size_t kPluginPathBufferSize = 512;

std::string defaultPluginPath(const char* executablePath) {
    namespace fs = std::filesystem;

    std::error_code error;
    fs::path executable = fs::absolute(fs::path(executablePath), error);
    if (error) {
        executable = fs::current_path(error);
    }

    fs::path baseDirectory = executable.has_parent_path() ? executable.parent_path()
                                                          : fs::path(".");

#if defined(_WIN32)
    const char* pluginFileName = "sample_plugin.dll";
#elif defined(__APPLE__)
    const char* pluginFileName = "libsample_plugin.dylib";
#else
    const char* pluginFileName = "libsample_plugin.so";
#endif

    const fs::path siblingPath = baseDirectory / pluginFileName;
    if (fs::exists(siblingPath, error) && !error) {
        return siblingPath.string();
    }

    const fs::path parentPath = baseDirectory.parent_path() / pluginFileName;
    if (fs::exists(parentPath, error) && !error) {
        return parentPath.string();
    }

    return siblingPath.string();
}

void copyStringToBuffer(const std::string& value, std::array<char, kPluginPathBufferSize>& buffer) {
    std::snprintf(buffer.data(), buffer.size(), "%s", value.c_str());
}

struct AppState {
    hotplugpp::PluginLoader loader;
    std::array<char, kPluginPathBufferSize> pluginPath = {};
    std::string status = "Idle. Choose a plugin path, then load it.";

    void refreshStatusAfterLoad(bool loaded) {
        status = loaded ? "Plugin loaded successfully." : "Failed to load plugin.";
    }

    void loadPlugin() {
        const std::string path(pluginPath.data());
        if (path.empty()) {
            status = "Enter a plugin path before loading.";
            return;
        }

        refreshStatusAfterLoad(loader.loadPlugin(path));
    }

    void unloadPlugin() {
        if (!loader.isLoaded()) {
            status = "No plugin is currently loaded.";
            return;
        }

        loader.unloadPlugin();
        status = "Plugin unloaded.";
    }

    void checkAndReload() {
        if (!loader.isLoaded()) {
            status = "Load a plugin before checking for reloads.";
            return;
        }

        const bool wasLoaded = loader.isLoaded();
        const bool reloaded = loader.checkAndReload();

        if (reloaded) {
            if (status.empty()) {
                status = "Plugin reloaded.";
            }
            return;
        }

        if (wasLoaded && !loader.isLoaded()) {
            status = "Reload failed. The plugin is now unloaded.";
            return;
        }

        status = "No plugin changes detected.";
    }
};

int runSmokeTest(const char* executablePath) {
    AppState state;
    copyStringToBuffer(defaultPluginPath(executablePath), state.pluginPath);
    state.loader.setReloadCallback([&state]() { state.status = "Plugin reloaded."; });

    state.loadPlugin();
    if (!state.loader.isLoaded()) {
        std::fprintf(stderr, "Smoke test failed: plugin did not load from '%s'.\n",
                     state.pluginPath.data());
        return 2;
    }

    state.checkAndReload();
    if (!state.loader.isLoaded()) {
        std::fprintf(stderr, "Smoke test failed: plugin became unloaded after check/reload.\n");
        return 3;
    }

    state.unloadPlugin();
    if (state.loader.isLoaded()) {
        std::fprintf(stderr, "Smoke test failed: plugin did not unload.\n");
        return 4;
    }

    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    const char* executablePath = argc > 0 ? argv[0] : "";
    if (argc > 1 && std::string(argv[1]) == "--smoke-test") {
        return runSmokeTest(executablePath);
    }

    if (!glfwInit()) {
        return 1;
    }

    const char* glslVersion = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(960, 540, "HotPlugPP ImGui Host Sample", nullptr,
                                          nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    if (!ImGui_ImplOpenGL3_Init(glslVersion)) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    AppState state;
    copyStringToBuffer(defaultPluginPath(executablePath), state.pluginPath);
    state.loader.setReloadCallback(
        [&state]() { state.status = "Plugin reloaded after a file change."; });

    auto previousFrame = std::chrono::steady_clock::now();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const auto currentFrame = std::chrono::steady_clock::now();
        const std::chrono::duration<float> delta = currentFrame - previousFrame;
        previousFrame = currentFrame;

        if (hotplugpp::IPlugin* plugin = state.loader.getPlugin()) {
            plugin->onUpdate(delta.count());
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(720.0f, 0.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Plugin Host");

        ImGui::TextWrapped(
            "Load, unload, and manually check hot-reload for a plugin shared library.");
        ImGui::Separator();

        ImGui::TextUnformatted("Plugin path");
        ImGui::InputText("##plugin-path", state.pluginPath.data(), state.pluginPath.size());

        if (ImGui::Button("Load")) {
            state.loadPlugin();
        }
        ImGui::SameLine();
        if (ImGui::Button("Unload")) {
            state.unloadPlugin();
        }
        ImGui::SameLine();
        if (ImGui::Button("Check / Reload")) {
            state.checkAndReload();
        }

        ImGui::Separator();

        ImGui::Text("State: %s", state.loader.isLoaded() ? "Loaded" : "Unloaded");

        if (hotplugpp::IPlugin* plugin = state.loader.getPlugin()) {
            ImGui::Text("Name: %s", plugin->getName());
            ImGui::Text("Version: %s", plugin->getVersion().toString().c_str());
            ImGui::TextWrapped("Description: %s", plugin->getDescription());
        } else {
            ImGui::TextUnformatted("Name: -");
            ImGui::TextUnformatted("Version: -");
            ImGui::TextUnformatted("Description: -");
        }

        ImGui::Spacing();
        ImGui::TextWrapped("Status: %s", state.status.c_str());

        ImGui::End();

        ImGui::Render();

        int displayWidth = 0;
        int displayHeight = 0;
        glfwGetFramebufferSize(window, &displayWidth, &displayHeight);
        glViewport(0, 0, displayWidth, displayHeight);
        glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    state.loader.unloadPlugin();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
