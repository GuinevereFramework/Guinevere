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
                                  guinevere::app::Context& frame_context,
                                  guinevere::ui::UiRuntime& runtime,
                                  const guinevere::ui::AppScaffoldResult& scaffold
                              ) -> bool {
            guinevere::ui::ComponentScope component =
                runtime.root_component("demo_window");
            const int click_count = component.use_auto<int>(0, "click_count");

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

            auto title_entry = component.label_auto("Guinevere Demo Window", "title");
            title_entry.layout(header_lines.start);
            auto subtitle_entry = component.label_auto(subtitle_text, "subtitle");
            subtitle_entry.layout(header_lines.end);
            if(show_visual_column && frame_context.assets.has_image("demo_hero")) {
                auto hero_entry = component.image_asset_auto("demo_hero", "hero_image");
                hero_entry.layout(hero_rect);
            }
            auto controls_panel_entry = component.panel(controls_panel_key);
            controls_panel_entry.layout(controls_panel_rect);
            controls_panel_entry.column(14.0f, 18.0f);
            controls_panel_entry.align_stretch();
            controls_panel_entry.justify_start();
            component.label_auto(controls_panel_key, "Controls Panel (Auto Layout)", "controls_label");
            auto button_row_entry = component.row(controls_panel_key, button_row_key, 20.0f, 0.0f);
            button_row_entry.height_fixed(64.0f);
            button_row_entry.overflow_scroll();
            button_row_entry.align_stretch();
            button_row_entry.justify_start();

            auto increase_button_entry =
                component.button_auto(button_row_key, "Click me", "counter_increase");
            increase_button_entry.on_click([component]() mutable {
                component.update_auto<int>([](int& value) {
                    ++value;
                }, "click_count");
            });
            increase_button_entry.width(guinevere::ui::ResponsiveProperty{
                .compact = 192.0f,
                .expanded = 240.0f
            });
            increase_button_entry.height_fixed(56.0f);

            auto reset_button_entry = component.button_auto(button_row_key, "Reset", "counter_reset");
            reset_button_entry.on_click([component]() mutable {
                component.set_auto<int>(0, "click_count");
            });
            reset_button_entry.width(guinevere::ui::ResponsiveProperty{
                .compact = 192.0f,
                .expanded = 240.0f
            });
            reset_button_entry.height_fixed(56.0f);

            auto info_button_entry = component.button_auto(button_row_key, "More", "counter_info");
            info_button_entry.width(guinevere::ui::ResponsiveProperty{
                .compact = 192.0f,
                .expanded = 240.0f
            });
            info_button_entry.height_fixed(56.0f);

            component.label_auto(
                controls_panel_key,
                "Button clicks: " + std::to_string(click_count),
                "counter_text"
            );
            component.label_auto(
                controls_panel_key,
                "Scroll mouse wheel over button row to pan horizontally",
                "counter_hint"
            );

            frame_context.renderer.clear(guinevere::gfx::Color{0.08f, 0.10f, 0.13f, 1.0f});

            if(show_visual_column) {
                const guinevere::ui::RectSplit after_hero =
                    guinevere::ui::split_column_start(visual_column_rect, hero_rect.h, 20.0f);
                const guinevere::gfx::Rect deco_space = after_hero.end;
                const float deco_w = std::min(deco_space.w, 320.0f);
                const float deco_h = std::min(
                    std::max(120.0f, deco_space.h - 8.0f),
                    180.0f
                );
                const guinevere::gfx::Rect blue_block = guinevere::ui::split_column_start(
                    guinevere::ui::split_row_start(deco_space, deco_w).start,
                    deco_h
                ).start;
                frame_context.renderer.fill_rect(
                    blue_block,
                    guinevere::gfx::Color{0.20f, 0.52f, 0.92f, 1.0f}
                );
                frame_context.renderer.stroke_rect(
                    blue_block,
                    guinevere::gfx::Color{0.92f, 0.96f, 1.0f, 1.0f},
                    3.0f
                );
                frame_context.renderer.push_clip(guinevere::gfx::Rect{
                    blue_block.x + 28.0f,
                    blue_block.y + 20.0f,
                    std::max(0.0f, blue_block.w - 88.0f),
                    std::max(0.0f, blue_block.h - 56.0f)
                });
                frame_context.renderer.fill_rect(
                    guinevere::gfx::Rect{
                        blue_block.x - 40.0f,
                        blue_block.y - 10.0f,
                        blue_block.w + 80.0f,
                        blue_block.h + 40.0f
                    },
                    guinevere::gfx::Color{0.92f, 0.32f, 0.27f, 1.0f}
                );
                frame_context.renderer.pop_clip();
            }
            return true;
    };

    callbacks.on_frame = [&](guinevere::app::Context& context) {
        const guinevere::gfx::Rect viewport = ui_runtime.viewport(context);
        return ui_runtime.app_frame(
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
            render_frame
        );
    };

    const guinevere::app::ErrorCallback callbacks_error = [](std::string_view message) {
        std::cerr << "[guinevere::app::run] " << message << '\n';
    };

    return guinevere::app::run(config, callbacks, callbacks_error);
}
