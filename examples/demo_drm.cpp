#include <iostream>
#include <string>
#include <vector>

#include <guinevere/guinevere.hpp>

namespace {

class CounterPanel {
public:
    explicit CounterPanel(std::string title)
        : title_(std::move(title))
    {
    }

    void render(guinevere::ui::ComponentScope& component) const
    {
        auto local_state = component.state();
        const int click_count = local_state.use<int>("click_count", 0);

        component
            .panel("panel")
            .column(10.0f, 14.0f)
            .align_center()
            .justify_start()
            .width_fill()
            .height_fill()
            .min_width(220.0f);
        component.label("panel", "title", title_);
        component
            .button("panel", "increase", "Increase")
            .on_click([local_state]() mutable {
                local_state.update<int>("click_count", [](int& value) {
                    ++value;
                });
            })
            .width(guinevere::ui::ResponsiveProperty{
                .compact = 160.0f,
                .expanded = 210.0f
            })
            .height_fixed(56.0f);
        component
            .button("panel", "reset", "Reset")
            .on_click([local_state]() mutable {
                local_state.set<int>("click_count", 0);
            })
            .width(guinevere::ui::ResponsiveProperty{
                .compact = 160.0f,
                .expanded = 210.0f
            })
            .height_fixed(56.0f);
        component.label("panel", "value", "Count: " + std::to_string(click_count));
    }

private:
    std::string title_;
};

class TextEditorPanel {
public:
    void render(guinevere::ui::ComponentScope& component) const
    {
        auto local_state = component.state();
        const std::string& text_value_utf8 = local_state.use<std::string>(
            "text_value_utf8",
            std::string("Type UTF-8 text here")
        );
        const std::string& submitted_utf8 = local_state.use<std::string>(
            "submitted_utf8",
            std::string{}
        );
        const std::string submitted_label = submitted_utf8.empty()
            ? std::string("Press Enter in TextEdit to submit")
            : std::string("Last submitted: ") + submitted_utf8;

        component
            .panel("panel")
            .column(12.0f, 14.0f)
            .align_stretch()
            .justify_start()
            .width_fill()
            .height_fill()
            .min_height(172.0f);
        component.label("panel", "title", "Text Input Component");
        component
            .text_edit("panel", "editor", text_value_utf8)
            .allow_caret_toggle()
            .height_fixed(56.0f)
            .width_fill()
            .on_text_change([local_state](const std::string& changed_value_utf8) mutable {
                local_state.set<std::string>("text_value_utf8", changed_value_utf8);
            })
            .on_text_submit([local_state](const std::string& submitted_value_utf8) mutable {
                local_state.set<std::string>("submitted_utf8", submitted_value_utf8);
            });
        component.label("panel", "submitted", submitted_label);
        component.label(
            "panel",
            "hint",
            "Insert: toggle caret | Left/Right: move | Up/Down: start/end"
        );
    }
};

void render_hint_component(guinevere::ui::ComponentScope& component)
{
    auto local_state = component.state();
    const bool expanded = local_state.use<bool>("expanded", false);

    component
        .panel("panel")
        .column(10.0f, 12.0f)
        .align_stretch()
        .justify_start()
        .width_fill();
    component
        .button("panel", "toggle", expanded ? "Hide component tips" : "Show component tips")
        .on_click([local_state]() mutable {
            local_state.update<bool>("expanded", [](bool& value) {
                value = !value;
            });
        })
        .height_fixed(56.0f)
        .width(guinevere::ui::ResponsiveProperty{
            .compact = 220.0f,
            .expanded = 280.0f
        });
    component.label(
        "panel",
        "content",
        expanded
            ? std::string(
                  "Function component keeps local state with its own namespace."
              )
            : std::string("Tips are hidden.")
    );
}

} // namespace

int main()
{
    guinevere::ui::UiRuntime ui_runtime("root");
    CounterPanel left_counter("Counter A (Class Component)");
    CounterPanel right_counter("Counter B (Class Component)");
    TextEditorPanel text_editor;
    ui_runtime.reserve(24U);

    guinevere::app::RunConfig config;
    config.width = 960;
    config.height = 820;
    config.title = "Guinevere DRM Demo";
    config.backend = guinevere::gfx::Backend::OpenGL;

    guinevere::app::Callbacks callbacks;

    const auto render_frame = [&](
                                  guinevere::app::Context& frame_context,
                                  guinevere::ui::UiRuntime& runtime,
                                  const guinevere::ui::AppScaffoldResult& scaffold
                              ) -> bool {
            const guinevere::gfx::Rect layout_bounds = scaffold.body;

            const float layout_padding = 18.0f;
            const float min_detail_height = 260.0f;
            const float footer_height = 36.0f;
            const float row_gap = 16.0f;
            const float preferred_counter_row_height = guinevere::ui::resolve_responsive_scalar(
                scaffold.app_layout.breakpoint,
                guinevere::ui::ResponsiveScalar{216.0f, 236.0f, 250.0f}
            );
            const float max_counter_row_height = guinevere::ui::axis_available_size(
                layout_bounds.h,
                layout_padding,
                min_detail_height + footer_height,
                row_gap,
                2U
            );
            const float counter_row_height = guinevere::ui::resolve_axis_size(
                preferred_counter_row_height,
                156.0f,
                max_counter_row_height
            );
            const std::string footer_text =
                "Each component keeps local state and local node keys. Breakpoint: "
                + std::string(guinevere::ui::app_breakpoint_label(scaffold.app_layout.breakpoint));
            const guinevere::gfx::Rect title_rect = guinevere::ui::inset_rect(
                scaffold.header,
                guinevere::ui::EdgeInsets{20.0f, 0.0f, 0.0f, 0.0f}
            );

            guinevere::ui::ComponentScope app_component =
                runtime.root_component("demo_drm");

            app_component
                .label("title", "Guinevere DRM Component Demo")
                .layout(title_rect);
            app_component
                .panel("layout_root")
                .layout(layout_bounds)
                .column(16.0f, 18.0f)
                .align_stretch()
                .justify_start();
            app_component
                .row("layout_root", "counter_row", 16.0f, 0.0f)
                .height_fixed(counter_row_height)
                .align_stretch()
                .justify_start();
            app_component
                .column("layout_root", "detail_column", 12.0f, 0.0f)
                .height_fill()
                .align_stretch()
                .justify_start();

            guinevere::ui::ComponentScope left_counter_component =
                app_component.mount("counter_left", "counter_row");
            guinevere::ui::ComponentScope right_counter_component =
                app_component.mount("counter_right", "counter_row");
            guinevere::ui::ComponentScope text_editor_component =
                app_component.mount("text_editor", "detail_column");
            guinevere::ui::ComponentScope hint_component =
                app_component.mount("hint", "detail_column");

            left_counter.render(left_counter_component);
            right_counter.render(right_counter_component);
            text_editor.render(text_editor_component);
            render_hint_component(hint_component);

            app_component
                .label(
                    "layout_root",
                    "footer",
                    footer_text
                )
                .height_fixed(36.0f)
                .align_center()
                .justify_start();

            frame_context.renderer.clear(guinevere::gfx::Color{0.08f, 0.10f, 0.13f, 1.0f});
            return true;
    };

    callbacks.on_frame = [&](guinevere::app::Context& context) {
        const guinevere::gfx::Rect viewport = ui_runtime.viewport(context);
        return ui_runtime.app_frame(
            context,
            guinevere::ui::AppScaffoldSpec{
                .app_layout = guinevere::ui::AppLayoutSpec{
                    .min_width = 320.0f,
                    .min_height = 580.0f,
                    .max_height = viewport.h - 20.0f,
                    .fill_viewport_height_below = 120.0f
                },
                .header_height = guinevere::ui::ResponsiveScalar{34.0f, 36.0f, 38.0f},
                .header_gap = guinevere::ui::ResponsiveScalar{10.0f, 12.0f, 14.0f}
            },
            render_frame
        );
    };

    const guinevere::app::ErrorCallback callbacks_error = [](std::string_view message) {
        std::cerr << "[guinevere::app::run] " << message << '\n';
    };

    return guinevere::app::run(config, callbacks, callbacks_error);
}
