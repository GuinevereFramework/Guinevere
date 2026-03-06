#include <iostream>
#include <string>

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
        const int click_count = component.state().use<int>("click_count", 0);
        const std::string panel_key = component.auto_local_key("panel");

        auto panel_entry = component.panel({}, panel_key);
        panel_entry.column(10.0f, 14.0f);
        panel_entry.align_center();
        panel_entry.justify_start();
        panel_entry.min_width(220.0f);
        component.label(panel_key, title_);

        auto increase_button_entry = component.button(panel_key, "Increase");
        increase_button_entry.on_click([component]() mutable {
            component.state().update<int>("click_count", [](int& value) {
                ++value;
            });
        });

        auto reset_button_entry = component.button(panel_key, "Reset");
        reset_button_entry.on_click([component]() mutable {
            component.state().set<int>("click_count", 0);
        });
        component.label(panel_key, "Count: " + std::to_string(click_count));
    }

private:
    std::string title_;
};

void render_hint_component(guinevere::ui::ComponentScope& component)
{
    const bool expanded = component.state().use<bool>("expanded", false);
    const std::string panel_key = component.auto_local_key("panel");

    auto panel_entry = component.panel({}, panel_key);
    panel_entry.column(10.0f, 12.0f);
    panel_entry.align_stretch();
    panel_entry.justify_start();

    auto toggle_button_entry =
        component.button(panel_key, expanded ? "Hide component tips" : "Show component tips");
    toggle_button_entry.on_click([component]() mutable {
        component.state().update<bool>("expanded", [](bool& value) {
            value = !value;
        });
    });
    component.label(
        panel_key,
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
    ui_runtime.reserve(20U);

    guinevere::app::RunConfig config;
    config.width = 960;
    config.height = 820;
    config.title = "Guinevere DRM Demo";
    config.backend = guinevere::gfx::Backend::OpenGL;

    guinevere::app::Callbacks callbacks;

    const auto render_frame = [&](
                                  guinevere::ui::ComponentScope& app_component,
                                  const guinevere::ui::AppScaffoldResult& scaffold
                              ) -> bool {
            const float min_detail_height = 260.0f;
            const float footer_height = 34.0f;
            const float row_gap = 16.0f;
            const float preferred_counter_row_height = guinevere::ui::resolve_responsive_scalar(
                scaffold.app_layout.breakpoint,
                guinevere::ui::ResponsiveScalar{216.0f, 236.0f, 250.0f}
            );
            const std::string footer_text =
                "Each component keeps local state and local node keys. Breakpoint: "
                + std::string(guinevere::ui::app_breakpoint_label(scaffold.app_layout.breakpoint));

            const std::string header_key = app_component.auto_local_key("header");
            const std::string layout_root_key = app_component.auto_local_key("layout_root");
            const std::string counter_row_key = app_component.auto_local_key("counter_row");
            const std::string detail_column_key = app_component.auto_local_key("detail_column");

            auto header_entry = app_component.column({}, header_key, 0.0f, 0.0f);
            header_entry.layout(scaffold.header);
            header_entry.align_start();
            header_entry.justify_start();
            (void)app_component.label(header_key, "Guinevere DRM Component Demo");

            auto layout_root_entry = app_component.panel({}, layout_root_key);
            layout_root_entry.layout(scaffold.body);
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
            counter_row_entry.main_axis_tracks({
                guinevere::ui::Flex(1.0f).min(220.0f).pref(280.0f),
                guinevere::ui::Flex(1.0f).min(220.0f).pref(280.0f)
            });

            auto detail_column_entry =
                app_component.column(layout_root_key, detail_column_key, 12.0f, 0.0f);
            detail_column_entry.align_stretch();
            detail_column_entry.justify_start();

            app_component.mount_component(
                app_component.auto_local_key("counter"),
                counter_row_key,
                left_counter
            );
            app_component.mount_component(
                app_component.auto_local_key("counter"),
                counter_row_key,
                right_counter
            );
            app_component.mount_invoke(
                app_component.auto_local_key("hint"),
                detail_column_key,
                render_hint_component
            );

            auto footer_entry = app_component.label(layout_root_key, footer_text);
            footer_entry.align_center();
            footer_entry.justify_start();

            return true;
    };

    callbacks.on_frame = [&](guinevere::app::Context& context) {
        const guinevere::gfx::Rect viewport = ui_runtime.viewport(context);
        return ui_runtime.component_frame(
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
            "demo_drm",
            render_frame,
            guinevere::gfx::Color{0.08f, 0.10f, 0.13f, 1.0f}
        );
    };

    const guinevere::app::ErrorCallback callbacks_error = [](std::string_view message) {
        std::cerr << "[guinevere::app::run] " << message << '\n';
    };

    return guinevere::app::run(config, callbacks, callbacks_error);
}
