#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <guinevere/asset/asset_manager.hpp>
#include <guinevere/gfx/renderer_factory.hpp>
#include <guinevere/platform/glfw_window.hpp>

namespace guinevere::app {

struct RunConfig {
    int width = 960;
    int height = 640;
    std::string title = "Guinevere App";
    gfx::Backend backend = gfx::Backend::OpenGL;
    bool auto_present = true;
    std::vector<std::string> asset_search_paths;
};

struct FrameInfo {
    std::uint64_t frame_index = 0;
    double elapsed_seconds = 0.0;
    double delta_seconds = 0.0;
};

struct Context {
    platform::GlfwWindow& window;
    gfx::Renderer& renderer;
    asset::AssetManager& assets;
    FrameInfo frame;
};

struct Callbacks {
    std::function<void(Context&)> on_init;
    std::function<bool(Context&)> on_frame;
    std::function<void(Context&)> on_shutdown;
    std::function<void(std::string_view)> on_error;
};

using ErrorCallback = std::function<void(std::string_view)>;

int run(const RunConfig& config,
        const Callbacks& callbacks,
        ErrorCallback on_error = {});

} // namespace guinevere::app
