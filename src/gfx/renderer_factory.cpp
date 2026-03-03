#include <stdexcept>
#include <utility>

#include <guinevere/gfx/renderer_factory.hpp>

namespace guinevere::gfx {

std::unique_ptr<Renderer> create_opengl_renderer(platform::GlfwWindow& window);

std::unique_ptr<Renderer> create_renderer(Backend backend, platform::GlfwWindow& window)
{
    switch(backend) {
        case Backend::OpenGL:
            return create_opengl_renderer(window);
    }

    throw std::runtime_error("Unsupported backend");
}

} // namespace guinevere::gfx
