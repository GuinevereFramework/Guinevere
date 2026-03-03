#pragma once

#include <memory>

#include <guinevere/gfx/renderer.hpp>

namespace guinevere::platform {
class GlfwWindow;
}

namespace guinevere::gfx {

enum class Backend {
    OpenGL,
};

std::unique_ptr<Renderer> create_renderer(Backend backend, platform::GlfwWindow& window);

} // namespace guinevere::gfx
