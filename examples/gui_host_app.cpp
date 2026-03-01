#define WIN32_LEAN_AND_MEAN
#include "hotplugpp/plugin_loader.hpp"

#include <chrono>
#include <string>
#include <vector>

#include <windows.h>

namespace {

constexpr int kTimerId = 1001;
constexpr int kWindowWidth = 560;
constexpr int kWindowHeight = 300;

enum ControlId {
    kPluginPathLabel = 2001,
    kPluginPathEdit,
    kLoadButton,
    kUnloadButton,
    kReloadButton,
    kStateValue,
    kNameValue,
    kVersionValue,
    kStatusValue
};

struct AppState {
    hotplugpp::PluginLoader loader;
    HWND pluginPathEdit = nullptr;
    HWND stateValue = nullptr;
    HWND nameValue = nullptr;
    HWND versionValue = nullptr;
    HWND statusValue = nullptr;
    std::string statusMessage = "Idle. Load a plugin to begin.";
    std::chrono::steady_clock::time_point lastUpdate = std::chrono::steady_clock::now();

    AppState() {
        loader.setReloadCallback(
            [this]() { statusMessage = "Plugin reloaded after a file change."; });
    }
};

std::string getExecutableDirectory() {
    std::vector<char> pathBuffer(MAX_PATH, '\0');
    DWORD length = GetModuleFileNameA(nullptr, pathBuffer.data(),
                                      static_cast<DWORD>(pathBuffer.size()));
    if (length == 0 || length >= pathBuffer.size()) {
        return ".";
    }

    std::string path(pathBuffer.data(), length);
    const std::size_t separator = path.find_last_of("\\/");
    if (separator == std::string::npos) {
        return ".";
    }

    return path.substr(0, separator);
}

std::string getDefaultPluginPath() {
    return getExecutableDirectory() + "\\sample_plugin.dll";
}

std::string readWindowText(HWND control) {
    const int length = GetWindowTextLengthA(control);
    if (length <= 0) {
        return {};
    }

    std::vector<char> buffer(static_cast<std::size_t>(length) + 1U, '\0');
    GetWindowTextA(control, buffer.data(), length + 1);
    return std::string(buffer.data());
}

void setWindowText(HWND control, const std::string& value) {
    SetWindowTextA(control, value.c_str());
}

void updatePluginDetails(AppState& state) {
    if (state.loader.isLoaded()) {
        hotplugpp::IPlugin* plugin = state.loader.getPlugin();
        if (plugin != nullptr) {
            setWindowText(state.stateValue, "Loaded");
            setWindowText(state.nameValue, plugin->getName());
            setWindowText(state.versionValue, plugin->getVersion().toString());
        } else {
            setWindowText(state.stateValue, "Loaded (plugin unavailable)");
            setWindowText(state.nameValue, "-");
            setWindowText(state.versionValue, "-");
        }
    } else {
        setWindowText(state.stateValue, "Unloaded");
        setWindowText(state.nameValue, "-");
        setWindowText(state.versionValue, "-");
    }

    setWindowText(state.statusValue, state.statusMessage);
}

void loadPlugin(AppState& state) {
    const std::string pluginPath = readWindowText(state.pluginPathEdit);
    if (pluginPath.empty()) {
        state.statusMessage = "Enter a plugin path before loading.";
        updatePluginDetails(state);
        return;
    }

    if (state.loader.loadPlugin(pluginPath)) {
        state.statusMessage = "Plugin loaded successfully.";
    } else {
        state.statusMessage = "Failed to load plugin.";
    }

    updatePluginDetails(state);
}

void unloadPlugin(AppState& state) {
    if (!state.loader.isLoaded()) {
        state.statusMessage = "No plugin is currently loaded.";
        updatePluginDetails(state);
        return;
    }

    state.loader.unloadPlugin();
    state.statusMessage = "Plugin unloaded.";
    updatePluginDetails(state);
}

void checkAndReload(AppState& state) {
    if (!state.loader.isLoaded()) {
        state.statusMessage = "Load a plugin before checking for reloads.";
        updatePluginDetails(state);
        return;
    }

    const bool wasLoaded = state.loader.isLoaded();
    const bool reloaded = state.loader.checkAndReload();

    if (reloaded) {
        if (state.statusMessage.empty()) {
            state.statusMessage = "Plugin reloaded.";
        }
    } else if (wasLoaded && !state.loader.isLoaded()) {
        state.statusMessage = "Reload failed. The plugin is now unloaded.";
    } else {
        state.statusMessage = "No plugin changes detected.";
    }

    updatePluginDetails(state);
}

LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    AppState* state = reinterpret_cast<AppState*>(GetWindowLongPtrA(window, GWLP_USERDATA));

    switch (message) {
    case WM_NCCREATE: {
        CREATESTRUCTA* createStruct = reinterpret_cast<CREATESTRUCTA*>(lParam);
        SetWindowLongPtrA(window, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
        return TRUE;
    }
    case WM_CREATE: {
        state = reinterpret_cast<AppState*>(
            reinterpret_cast<CREATESTRUCTA*>(lParam)->lpCreateParams);

        CreateWindowExA(0, "STATIC", "Plugin path:", WS_CHILD | WS_VISIBLE, 20, 20, 90, 20, window,
                        reinterpret_cast<HMENU>(kPluginPathLabel), nullptr, nullptr);

        state->pluginPathEdit = CreateWindowExA(
            WS_EX_CLIENTEDGE, "EDIT", getDefaultPluginPath().c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 110, 18, 330, 24, window,
            reinterpret_cast<HMENU>(kPluginPathEdit), nullptr, nullptr);

        CreateWindowExA(0, "BUTTON", "Load", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 450, 18, 80, 24,
                        window, reinterpret_cast<HMENU>(kLoadButton), nullptr, nullptr);
        CreateWindowExA(0, "BUTTON", "Unload", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 110, 58, 80, 24,
                        window, reinterpret_cast<HMENU>(kUnloadButton), nullptr, nullptr);
        CreateWindowExA(0, "BUTTON", "Check/Reload", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 200, 58,
                        110, 24, window, reinterpret_cast<HMENU>(kReloadButton), nullptr, nullptr);

        CreateWindowExA(0, "STATIC", "State:", WS_CHILD | WS_VISIBLE, 20, 108, 80, 20, window,
                        nullptr, nullptr, nullptr);
        state->stateValue = CreateWindowExA(0, "STATIC", "Unloaded", WS_CHILD | WS_VISIBLE, 110,
                                            108, 420, 20, window,
                                            reinterpret_cast<HMENU>(kStateValue), nullptr, nullptr);

        CreateWindowExA(0, "STATIC", "Name:", WS_CHILD | WS_VISIBLE, 20, 138, 80, 20, window,
                        nullptr, nullptr, nullptr);
        state->nameValue = CreateWindowExA(0, "STATIC", "-", WS_CHILD | WS_VISIBLE, 110, 138, 420,
                                           20, window, reinterpret_cast<HMENU>(kNameValue), nullptr,
                                           nullptr);

        CreateWindowExA(0, "STATIC", "Version:", WS_CHILD | WS_VISIBLE, 20, 168, 80, 20, window,
                        nullptr, nullptr, nullptr);
        state->versionValue = CreateWindowExA(
            0, "STATIC", "-", WS_CHILD | WS_VISIBLE, 110, 168, 420, 20, window,
            reinterpret_cast<HMENU>(kVersionValue), nullptr, nullptr);

        CreateWindowExA(0, "STATIC", "Status:", WS_CHILD | WS_VISIBLE, 20, 208, 80, 20, window,
                        nullptr, nullptr, nullptr);
        state->statusValue = CreateWindowExA(
            0, "STATIC", state->statusMessage.c_str(), WS_CHILD | WS_VISIBLE, 110, 208, 420, 40,
            window, reinterpret_cast<HMENU>(kStatusValue), nullptr, nullptr);

        SetTimer(window, kTimerId, 100, nullptr);
        updatePluginDetails(*state);
        return 0;
    }
    case WM_COMMAND:
        if (state == nullptr) {
            return 0;
        }

        switch (LOWORD(wParam)) {
        case kLoadButton:
            loadPlugin(*state);
            return 0;
        case kUnloadButton:
            unloadPlugin(*state);
            return 0;
        case kReloadButton:
            checkAndReload(*state);
            return 0;
        default:
            return 0;
        }
    case WM_TIMER:
        if (state != nullptr && wParam == kTimerId) {
            const auto now = std::chrono::steady_clock::now();
            const std::chrono::duration<float> delta = now - state->lastUpdate;
            state->lastUpdate = now;

            hotplugpp::IPlugin* plugin = state->loader.getPlugin();
            if (plugin != nullptr) {
                plugin->onUpdate(delta.count());
            }
        }
        return 0;
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        if (state != nullptr) {
            KillTimer(window, kTimerId);
            state->loader.unloadPlugin();
        }
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcA(window, message, wParam, lParam);
    }
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    const char* className = "HotPlugPPGuiHostWindow";

    WNDCLASSA windowClass = {};
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    if (RegisterClassA(&windowClass) == 0) {
        return 1;
    }

    AppState state;
    HWND window = CreateWindowExA(0, className, "HotPlugPP GUI Host Sample",
                                  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                  CW_USEDEFAULT, CW_USEDEFAULT, kWindowWidth, kWindowHeight,
                                  nullptr, nullptr, instance, &state);

    if (window == nullptr) {
        return 1;
    }

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message = {};
    while (GetMessageA(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return static_cast<int>(message.wParam);
}
