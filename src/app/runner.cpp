#include <chrono>
#include <exception>
#include <filesystem>
#include <memory>
#include <stdexcept>

#include <guinevere/app/runner.hpp>

namespace guinevere::app {

namespace {

void notify_error_handlers(std::string_view message,
                           const Callbacks& callbacks,
                           const ErrorCallback& on_error)
{
    if(callbacks.on_error) {
        callbacks.on_error(message);
    }

    if(on_error) {
        on_error(message);
    }
}

} // namespace

int run(const RunConfig& config, const Callbacks& callbacks, ErrorCallback on_error)
{
    try {
        if(config.width <= 0 || config.height <= 0) {
            throw std::runtime_error("Window size must be positive in guinevere::app::run");
        }

        platform::GlfwWindow window;
        window.create(config.width, config.height, config.title.c_str());

        std::unique_ptr<gfx::Renderer> renderer = gfx::create_renderer(config.backend, window);
        if(renderer == nullptr) {
            throw std::runtime_error("Renderer creation failed in guinevere::app::run");
        }

        asset::AssetManager assets;
        assets.add_search_path(std::filesystem::current_path());
        for(const std::string& search_path : config.asset_search_paths) {
            if(search_path.empty()) {
                continue;
            }
            assets.add_search_path(search_path);
        }

        Context context{
            window,
            *renderer,
            assets,
            FrameInfo{}
        };

        if(callbacks.on_init) {
            callbacks.on_init(context);
        }

        using Clock = std::chrono::steady_clock;
        const auto start_time = Clock::now();
        auto previous_time = start_time;

        while(window.is_open()) {
            window.poll_events();

            const auto now = Clock::now();
            context.frame.elapsed_seconds = std::chrono::duration<double>(now - start_time).count();
            context.frame.delta_seconds = std::chrono::duration<double>(now - previous_time).count();
            previous_time = now;

            bool keep_running = true;
            if(callbacks.on_frame) {
                keep_running = callbacks.on_frame(context);
            }

            if(config.auto_present) {
                renderer->present();
            }

            if(!keep_running) {
                break;
            }

            ++context.frame.frame_index;
        }

        if(callbacks.on_shutdown) {
            callbacks.on_shutdown(context);
        }

        return 0;
    } catch(const std::exception& error) {
        notify_error_handlers(error.what(), callbacks, on_error);
        return 1;
    } catch(...) {
        notify_error_handlers("Unknown exception in guinevere::app::run", callbacks, on_error);
        return 1;
    }
}

} // namespace guinevere::app
