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
        const int click_count = component.use_auto<int>(0, "click_count");
        const std::string panel_key = component.auto_local_key("panel");

        auto panel_entry = component.panel(panel_key);
        panel_entry.column(10.0f, 14.0f);
        panel_entry.align_center();
        panel_entry.justify_start();
        panel_entry.width_fill();
        panel_entry.height_fill();
        panel_entry.min_width(220.0f);
        component.label_auto(panel_key, title_, "title");

        auto increase_button_entry = component.button_auto(panel_key, "Increase", "increase");
        increase_button_entry.on_click([component]() mutable {
            component.update_auto<int>([](int& value) {
                ++value;
            }, "click_count");
        });
        increase_button_entry.width(guinevere::ui::ResponsiveProperty{
            .compact = 160.0f,
            .expanded = 210.0f
        });
        increase_button_entry.height_fixed(56.0f);

        auto reset_button_entry = component.button_auto(panel_key, "Reset", "reset");
        reset_button_entry.on_click([component]() mutable {
            component.set_auto<int>(0, "click_count");
        });
        reset_button_entry.width(guinevere::ui::ResponsiveProperty{
            .compact = 160.0f,
            .expanded = 210.0f
        });
        reset_button_entry.height_fixed(56.0f);
        component.label_auto(
            panel_key,
            "Count: " + std::to_string(click_count),
            "value"
        );
    }

private:
    std::string title_;
};

class TextEditorPanel {
public:
    void render(guinevere::ui::ComponentScope& component) const
    {
        const std::string& text_value_utf8 = component.use_auto<std::string>(
            std::string("Type UTF-8 text here"),
            "text_value_utf8"
        );
        const std::string& submitted_utf8 =
            component.use_auto<std::string>(std::string{}, "submitted_utf8");
        const std::string submitted_label = submitted_utf8.empty()
            ? std::string("Press Enter in TextEdit to submit")
            : std::string("Last submitted: ") + submitted_utf8;
        const std::string panel_key = component.auto_local_key("panel");

        auto panel_entry = component.panel(panel_key);
        panel_entry.column(12.0f, 14.0f);
        panel_entry.align_stretch();
        panel_entry.justify_start();
        panel_entry.width_fill();
        panel_entry.height_fill();
        panel_entry.min_height(172.0f);
        component.label_auto(panel_key, "Text Input Component", "title");
        auto text_edit_entry = component.text_edit_auto(panel_key, text_value_utf8, "editor");
        text_edit_entry.allow_caret_toggle();
        text_edit_entry.height_fixed(56.0f);
        text_edit_entry.width_fill();
        text_edit_entry.on_text_change([component](const std::string& changed_value_utf8) mutable {
            component.set_auto<std::string>(changed_value_utf8, "text_value_utf8");
        });
        text_edit_entry.on_text_submit([component](const std::string& submitted_value_utf8) mutable {
            component.set_auto<std::string>(submitted_value_utf8, "submitted_utf8");
        });
        component.label_auto(panel_key, submitted_label, "submitted");
        component.label_auto(
            panel_key,
            "Insert: toggle caret | Left/Right: move | Up/Down: start/end",
            "hint"
        );
    }
};

void render_hint_component(guinevere::ui::ComponentScope& component)
{
    const bool expanded = component.use_auto<bool>(false, "expanded");
    const std::string panel_key = component.auto_local_key("panel");

    auto panel_entry = component.panel(panel_key);
    panel_entry.column(10.0f, 12.0f);
    panel_entry.align_stretch();
    panel_entry.justify_start();
    panel_entry.width_fill();

    auto toggle_button_entry = component.button_auto(
        panel_key,
        expanded ? "Hide component tips" : "Show component tips",
        "toggle"
    );
    toggle_button_entry.on_click([component]() mutable {
        component.update_auto<bool>([](bool& value) {
            value = !value;
        }, "expanded");
    });
    toggle_button_entry.height_fixed(56.0f);
    toggle_button_entry.width(guinevere::ui::ResponsiveProperty{
        .compact = 220.0f,
        .expanded = 280.0f
    });
        component.label_auto(
            panel_key,
            expanded
                ? std::string(
                      "Function component keeps local state with its own namespace."
                  )
                : std::string("Tips are hidden."),
            "content"
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

            const float min_detail_height = 260.0f;
            const float footer_height = 36.0f;
            const float row_gap = 16.0f;
            const float preferred_counter_row_height = guinevere::ui::resolve_responsive_scalar(
                scaffold.app_layout.breakpoint,
                guinevere::ui::ResponsiveScalar{216.0f, 236.0f, 250.0f}
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
            const std::string layout_root_key = app_component.auto_local_key("layout_root");
            const std::string counter_row_key = app_component.auto_local_key("counter_row");
            const std::string detail_column_key = app_component.auto_local_key("detail_column");

            auto title_entry = app_component.label_auto("Guinevere DRM Component Demo", "title");
            title_entry.layout(title_rect);

            auto layout_root_entry = app_component.panel(layout_root_key);
            layout_root_entry.layout(layout_bounds);
            layout_root_entry.column(row_gap, 18.0f);
            layout_root_entry.main_axis_tracks({
                guinevere::ui::Flex(0.0f).min(156.0f).pref(preferred_counter_row_height).priority(1),
                guinevere::ui::Flex(1.0f).min(min_detail_height).pref(min_detail_height).priority(2),
                guinevere::ui::Fixed(footer_height).priority(3)
            });
            layout_root_entry.align_stretch();
            layout_root_entry.justify_start();

            auto counter_row_entry = app_component.row(layout_root_key, counter_row_key, row_gap, 0.0f);
            counter_row_entry.align_stretch();
            counter_row_entry.justify_start();

            auto detail_column_entry =
                app_component.column(layout_root_key, detail_column_key, 12.0f, 0.0f);
            detail_column_entry.height_fill();
            detail_column_entry.align_stretch();
            detail_column_entry.justify_start();

            app_component.mount_component_auto(counter_row_key, left_counter, "counter");
            app_component.mount_component_auto(counter_row_key, right_counter, "counter");
            app_component.mount_component_auto(detail_column_key, text_editor, "editor");
            app_component.mount_invoke_auto(detail_column_key, render_hint_component, "hint");

            auto footer_entry = app_component.label_auto(layout_root_key, footer_text, "footer");
            footer_entry.align_center();
            footer_entry.justify_start();

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
