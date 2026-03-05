#include <array>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <guinevere/guinevere.hpp>

namespace {

std::string find_demo_image()
{
    const std::array<const char*, 5> candidates = {{
        "../../../examples/assets/demo-image.png",
        "../../examples/assets/demo-image.png",
        "../examples/assets/demo-image.png",
        "examples/assets/demo-image.png",
        "C:/Windows/Web/Wallpaper/Windows/img0.jpg"
    }};

    for(const char* candidate : candidates) {
        if(std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return {};
}

} // namespace

int main()
{
    const std::string image_path = find_demo_image();

    guinevere::ui::UiRuntime ui_runtime("root");
    ui_runtime.reserve(12U);

    guinevere::app::RunConfig config;
    config.width = 960;
    config.height = 640;
    config.title = "Guinevere Demo Window (DRM)";
    config.backend = guinevere::gfx::Backend::OpenGL;

    guinevere::app::Callbacks callbacks;
    callbacks.on_init = [&image_path](guinevere::app::Context& context) {
        if(!image_path.empty()) {
            (void)context.assets.register_image("demo_hero", image_path);
        }
    };

    const auto render_frame = [&](
                                  guinevere::ui::ComponentScope& component,
                                  const guinevere::ui::AppScaffoldResult& scaffold
                              ) -> bool {
            const int click_count = component.state().use<int>("click_count", 0);

            const float body_gap = guinevere::ui::resolve_responsive_scalar(
                scaffold.app_layout.breakpoint,
                guinevere::ui::ResponsiveScalar{12.0f, 16.0f, 20.0f}
            );
            const bool show_visual_column =
                scaffold.app_layout.breakpoint != guinevere::ui::AppBreakpoint::Compact;
            const std::string subtitle_text =
                "Framework-connected reusable buttons (DRM) - "
                + std::string(guinevere::ui::app_breakpoint_label(scaffold.app_layout.breakpoint));
            const std::string header_key = component.auto_local_key("header");
            const std::string body_root_key = component.auto_local_key("body_root");
            const std::string body_content_key = component.auto_local_key("body_content");
            const std::string controls_panel_key = component.auto_local_key("controls_panel");
            const std::string button_grid_key = component.auto_local_key("button_grid");
            const std::string visual_column_key = component.auto_local_key("visual_column");
            const std::string deco_block_key = component.auto_local_key("deco_block");
            const std::string deco_inner_key = component.auto_local_key("deco_inner");

            auto header_entry = component.column({}, header_key, 2.0f, 0.0f);
            header_entry.layout(scaffold.header);
            header_entry.align_start();
            header_entry.justify_start();
            (void)component.label(header_key, "Guinevere Demo Window");
            (void)component.label(header_key, subtitle_text);

            auto body_root_entry = component.panel({}, body_root_key);
            body_root_entry.layout(scaffold.body);
            body_root_entry.column(0.0f, 0.0f);
            body_root_entry.align_stretch();
            body_root_entry.justify_start();

            std::string controls_parent_key = body_root_key;
            if(show_visual_column) {
                auto body_content_entry = component.row(body_root_key, body_content_key, body_gap, 0.0f);
                body_content_entry.align_stretch();
                body_content_entry.justify_start();
                body_content_entry.main_axis_tracks({
                    guinevere::ui::Flex(1.0f).min(300.0f).pref(460.0f),
                    guinevere::ui::Fixed(320.0f)
                });
                controls_parent_key = body_content_key;

                auto visual_column_entry =
                    component.column(body_content_key, visual_column_key, 20.0f, 0.0f);
                visual_column_entry.align_stretch();
                visual_column_entry.justify_start();

                auto hero_entry = component.image_asset(visual_column_key, "demo_hero");
                hero_entry.height(guinevere::ui::ResponsiveProperty{
                    .medium = 180.0f,
                    .expanded = 220.0f
                });

                auto deco_block_entry = component.panel(visual_column_key, deco_block_key);
                deco_block_entry.column(0.0f, 0.0f);
                deco_block_entry.overflow_clip();
                deco_block_entry.min_height(120.0f);

                auto deco_inner_entry = component.panel(deco_block_key, deco_inner_key);
                deco_inner_entry.height_fill();
                deco_inner_entry.min_width(180.0f);
                deco_inner_entry.min_height(90.0f);
            }

            auto controls_panel_entry = component.panel(controls_parent_key, controls_panel_key);
            controls_panel_entry.column(14.0f, 18.0f);
            controls_panel_entry.align_stretch();
            controls_panel_entry.justify_start();
            component.label(controls_panel_key, "Controls Panel (Auto Layout)");

            auto button_grid_entry = component.grid(
                controls_panel_key,
                button_grid_key,
                show_visual_column ? 3U : 1U,
                12.0f,
                0.0f
            );
            button_grid_entry.align_stretch();
            button_grid_entry.justify_start();
            if(show_visual_column) {
                button_grid_entry.grid_columns({
                    guinevere::ui::Flex(1.0f).min(140.0f).pref(160.0f),
                    guinevere::ui::Flex(1.0f).min(140.0f).pref(160.0f),
                    guinevere::ui::Flex(1.0f).min(140.0f).pref(160.0f)
                });
            }

            auto increase_button_entry = component.button(button_grid_key, "Click me");
            increase_button_entry.on_click([component]() mutable {
                component.state().update<int>("click_count", [](int& value) {
                    ++value;
                });
            });

            auto reset_button_entry = component.button(button_grid_key, "Reset");
            reset_button_entry.on_click([component]() mutable {
                component.state().set<int>("click_count", 0);
            });

            (void)component.button(button_grid_key, "More");

            component.label(
                controls_panel_key,
                "Button clicks: " + std::to_string(click_count)
            );
            component.label(
                controls_panel_key,
                "Auto grid + intrinsic button sizing (no manual split rectangles)"
            );
            return true;
    };

    callbacks.on_frame = [&](guinevere::app::Context& context) {
        const guinevere::gfx::Rect viewport = ui_runtime.viewport(context);
        return ui_runtime.component_frame(
            context,
            guinevere::ui::AppScaffoldSpec{
                .app_layout = guinevere::ui::AppLayoutSpec{
                    .min_width = 320.0f,
                    .min_height = 420.0f,
                    .max_height = viewport.h - 20.0f,
                    .fill_viewport_height_below = 120.0f
                },
                .header_height = guinevere::ui::ResponsiveScalar{72.0f, 74.0f, 76.0f},
                .header_gap = guinevere::ui::ResponsiveScalar{12.0f, 14.0f, 16.0f}
            },
            "demo_window",
            render_frame,
            guinevere::gfx::Color{0.08f, 0.10f, 0.13f, 1.0f}
        );
    };

    const guinevere::app::ErrorCallback callbacks_error = [](std::string_view message) {
        std::cerr << "[guinevere::app::run] " << message << '\n';
    };

    return guinevere::app::run(config, callbacks, callbacks_error);
}
