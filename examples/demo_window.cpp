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
            guinevere::gfx::Rect controls_panel_rect = scaffold.body;
            guinevere::gfx::Rect visual_column_rect{};
            if(show_visual_column) {
                const guinevere::ui::RectSplit body_columns = guinevere::ui::split_row_ratio_end(
                    scaffold.body,
                    0.34f,
                    body_gap,
                    220.0f,
                    360.0f
                );
                controls_panel_rect = body_columns.start;
                visual_column_rect = body_columns.end;
            }
            const float hero_height = guinevere::ui::resolve_axis_size(
                visual_column_rect.h * 0.52f,
                150.0f,
                240.0f
            );
            const guinevere::gfx::Rect hero_rect =
                guinevere::ui::split_column_start(visual_column_rect, hero_height).start;
            const std::string subtitle_text =
                "Framework-connected reusable buttons (DRM) - "
                + std::string(guinevere::ui::app_breakpoint_label(scaffold.app_layout.breakpoint));
            const guinevere::ui::RectSplit header_lines =
                guinevere::ui::split_column_start(scaffold.header, 34.0f, 2.0f);
            const std::string controls_panel_key = component.auto_local_key("controls_panel");
            const std::string button_row_key = component.auto_local_key("button_row");

            auto title_entry = component.label(
                component.auto_local_key("title"),
                "Guinevere Demo Window"
            );
            title_entry.layout(header_lines.start);
            auto subtitle_entry = component.label(
                component.auto_local_key("subtitle"),
                subtitle_text
            );
            subtitle_entry.layout(header_lines.end);
            if(show_visual_column) {
                auto hero_entry =
                    component.image_asset(component.auto_local_key("hero_image"), "demo_hero");
                hero_entry.layout(hero_rect);
            }
            auto controls_panel_entry = component.panel(controls_panel_key);
            controls_panel_entry.layout(controls_panel_rect);
            controls_panel_entry.column(14.0f, 18.0f);
            controls_panel_entry.align_stretch();
            controls_panel_entry.justify_start();
            component.label(
                controls_panel_key,
                component.auto_local_key("controls_label"),
                "Controls Panel (Auto Layout)"
            );
            auto button_row_entry = component.row(controls_panel_key, button_row_key, 20.0f, 0.0f);
            button_row_entry.height_fixed(64.0f);
            button_row_entry.overflow_scroll();
            button_row_entry.align_stretch();
            button_row_entry.justify_start();

            auto increase_button_entry =
                component.button(
                    button_row_key,
                    component.auto_local_key("counter_increase"),
                    "Click me"
                );
            increase_button_entry.on_click([component]() mutable {
                component.state().update<int>("click_count", [](int& value) {
                    ++value;
                });
            });
            increase_button_entry.width(guinevere::ui::ResponsiveProperty{
                .compact = 192.0f,
                .expanded = 240.0f
            });
            increase_button_entry.height_fixed(56.0f);

            auto reset_button_entry = component.button(
                button_row_key,
                component.auto_local_key("counter_reset"),
                "Reset"
            );
            reset_button_entry.on_click([component]() mutable {
                component.state().set<int>("click_count", 0);
            });
            reset_button_entry.width(guinevere::ui::ResponsiveProperty{
                .compact = 192.0f,
                .expanded = 240.0f
            });
            reset_button_entry.height_fixed(56.0f);

            auto info_button_entry = component.button(
                button_row_key,
                component.auto_local_key("counter_info"),
                "More"
            );
            info_button_entry.width(guinevere::ui::ResponsiveProperty{
                .compact = 192.0f,
                .expanded = 240.0f
            });
            info_button_entry.height_fixed(56.0f);

            component.label(
                controls_panel_key,
                component.auto_local_key("counter_text"),
                "Button clicks: " + std::to_string(click_count)
            );
            component.label(
                controls_panel_key,
                component.auto_local_key("counter_hint"),
                "Scroll mouse wheel over button row to pan horizontally"
            );

            if(show_visual_column) {
                const guinevere::ui::RectSplit after_hero =
                    guinevere::ui::split_column_start(visual_column_rect, hero_rect.h, 20.0f);
                const guinevere::gfx::Rect deco_space = after_hero.end;
                const float deco_w = std::min(deco_space.w, 320.0f);
                const float deco_h = std::min(
                    std::max(120.0f, deco_space.h - 8.0f),
                    180.0f
                );
                const guinevere::gfx::Rect deco_block = guinevere::ui::split_column_start(
                    guinevere::ui::split_row_start(deco_space, deco_w).start,
                    deco_h
                ).start;
                const std::string deco_block_key = component.auto_local_key("deco_block");
                const std::string deco_inner_key = component.auto_local_key("deco_inner");
                auto deco_block_entry = component.panel(deco_block_key);
                deco_block_entry.layout(deco_block);
                deco_block_entry.overflow_clip();
                auto deco_inner_entry = component.panel(deco_block_key, deco_inner_key);
                deco_inner_entry.layout(guinevere::gfx::Rect{
                    deco_block.x - 40.0f,
                    deco_block.y - 10.0f,
                    deco_block.w + 80.0f,
                    deco_block.h + 40.0f
                });
            }
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
