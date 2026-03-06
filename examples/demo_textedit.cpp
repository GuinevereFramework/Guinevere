#include <iostream>
#include <string>

#include <guinevere/guinevere.hpp>

namespace {

class TextEditorPanel {
public:
    void render(guinevere::ui::ComponentScope& component) const
    {
        const std::string& text_value_utf8 =
            component.state().use<std::string>("text_value_utf8", std::string("Type UTF-8 text here"));
        const std::string& submitted_utf8 =
            component.state().use<std::string>("submitted_utf8", std::string{});
        const bool allow_ctrl_a = component.state().use<bool>("allow_ctrl_a", true);
        const bool allow_ctrl_c = component.state().use<bool>("allow_ctrl_c", true);
        const bool allow_ctrl_v = component.state().use<bool>("allow_ctrl_v", true);
        const bool allow_ctrl_x = component.state().use<bool>("allow_ctrl_x", true);
        const bool allow_mouse_set_cursor =
            component.state().use<bool>("allow_mouse_set_cursor", true);
        const bool allow_mouse_drag_selection =
            component.state().use<bool>("allow_mouse_drag_selection", true);
        const bool custom_selection_background =
            component.state().use<bool>("custom_selection_background", false);
        const bool custom_selection_text =
            component.state().use<bool>("custom_selection_text", false);
        const std::string submitted_label = submitted_utf8.empty()
            ? std::string("Press Enter in TextEdit to submit")
            : std::string("Last submitted: ") + submitted_utf8;
        const std::string panel_key = component.auto_local_key("panel");

        auto panel_entry = component.panel({}, panel_key);
        panel_entry.column(12.0f, 14.0f);
        panel_entry.align_stretch();
        panel_entry.justify_start();
        panel_entry.min_height(172.0f);
        component.label(panel_key, "Text Input Component");
        auto text_edit_entry = component.text_edit(panel_key, text_value_utf8);
        text_edit_entry.allow_caret_toggle();
        text_edit_entry.allow_ctrl_a(allow_ctrl_a);
        text_edit_entry.allow_ctrl_c(allow_ctrl_c);
        text_edit_entry.allow_ctrl_v(allow_ctrl_v);
        text_edit_entry.allow_ctrl_x(allow_ctrl_x);
        text_edit_entry.allow_mouse_set_cursor(allow_mouse_set_cursor);
        text_edit_entry.allow_mouse_drag_selection(allow_mouse_drag_selection);
        if(custom_selection_background) {
            text_edit_entry.selection_background_color(
                guinevere::gfx::Color{0.16f, 0.32f, 0.74f, 0.95f}
            );
        }
        if(custom_selection_text) {
            text_edit_entry.selection_text_color(
                guinevere::gfx::Color{1.0f, 0.98f, 0.90f, 1.0f}
            );
        }
        text_edit_entry.on_text_change([component](const std::string& changed_value_utf8) mutable {
            component.state().set<std::string>("text_value_utf8", changed_value_utf8);
        });
        text_edit_entry.on_text_submit([component](const std::string& submitted_value_utf8) mutable {
            component.state().set<std::string>("submitted_utf8", submitted_value_utf8);
        });
        auto toggle_ctrl_a_button = component.button(
            panel_key,
            std::string("Ctrl+A (Select all): ") + (allow_ctrl_a ? "ON" : "OFF")
        );
        toggle_ctrl_a_button.on_click([component]() mutable {
            component.state().update<bool>("allow_ctrl_a", [](bool& value) {
                value = !value;
            });
        });

        auto toggle_ctrl_c_button = component.button(
            panel_key,
            std::string("Ctrl+C (Copy): ") + (allow_ctrl_c ? "ON" : "OFF")
        );
        toggle_ctrl_c_button.on_click([component]() mutable {
            component.state().update<bool>("allow_ctrl_c", [](bool& value) {
                value = !value;
            });
        });

        auto toggle_ctrl_v_button = component.button(
            panel_key,
            std::string("Ctrl+V (Paste): ") + (allow_ctrl_v ? "ON" : "OFF")
        );
        toggle_ctrl_v_button.on_click([component]() mutable {
            component.state().update<bool>("allow_ctrl_v", [](bool& value) {
                value = !value;
            });
        });

        auto toggle_ctrl_x_button = component.button(
            panel_key,
            std::string("Ctrl+X (Cut): ") + (allow_ctrl_x ? "ON" : "OFF")
        );
        toggle_ctrl_x_button.on_click([component]() mutable {
            component.state().update<bool>("allow_ctrl_x", [](bool& value) {
                value = !value;
            });
        });

        auto toggle_mouse_cursor_button = component.button(
            panel_key,
            std::string("Mouse click set cursor: ") + (allow_mouse_set_cursor ? "ON" : "OFF")
        );
        toggle_mouse_cursor_button.on_click([component]() mutable {
            component.state().update<bool>("allow_mouse_set_cursor", [](bool& value) {
                value = !value;
            });
        });

        auto toggle_mouse_drag_select_button = component.button(
            panel_key,
            std::string("Mouse drag select: ") + (allow_mouse_drag_selection ? "ON" : "OFF")
        );
        toggle_mouse_drag_select_button.on_click([component]() mutable {
            component.state().update<bool>("allow_mouse_drag_selection", [](bool& value) {
                value = !value;
            });
        });

        auto toggle_selection_background_button = component.button(
            panel_key,
            std::string("Selection background color: ")
                + (custom_selection_background ? "CUSTOM" : "DEFAULT")
        );
        toggle_selection_background_button.on_click([component]() mutable {
            component.state().update<bool>("custom_selection_background", [](bool& value) {
                value = !value;
            });
        });

        auto toggle_selection_text_button = component.button(
            panel_key,
            std::string("Selection text color: ") + (custom_selection_text ? "CUSTOM" : "DEFAULT")
        );
        toggle_selection_text_button.on_click([component]() mutable {
            component.state().update<bool>("custom_selection_text", [](bool& value) {
                value = !value;
            });
        });
        component.label(panel_key, submitted_label);
        component.label(
            panel_key,
            "Insert: toggle caret | Left/Right: move | Up/Down: start/end | Ctrl+A/C/V/X | Mouse drag: select"
        );
    }
};

} // namespace

int main()
{
    guinevere::ui::UiRuntime ui_runtime("root");
    TextEditorPanel text_editor;
    ui_runtime.reserve(14U);

    guinevere::app::RunConfig config;
    config.width = 900;
    config.height = 700;
    config.title = "Guinevere TextEdit Demo";
    config.backend = guinevere::gfx::Backend::OpenGL;

    guinevere::app::Callbacks callbacks;

    const auto render_frame = [&](
                                  guinevere::ui::ComponentScope& app_component,
                                  const guinevere::ui::AppScaffoldResult& scaffold
                              ) -> bool {
            const float footer_height = 34.0f;
            const std::string footer_text =
                "TextEdit supports submit on Enter and configurable shortcuts. Breakpoint: "
                + std::string(guinevere::ui::app_breakpoint_label(scaffold.app_layout.breakpoint));

            const std::string header_key = app_component.auto_local_key("header");
            const std::string layout_root_key = app_component.auto_local_key("layout_root");

            auto header_entry = app_component.column({}, header_key, 0.0f, 0.0f);
            header_entry.layout(scaffold.header);
            header_entry.align_start();
            header_entry.justify_start();
            (void)app_component.label(header_key, "Guinevere TextEdit Demo");

            auto layout_root_entry = app_component.panel({}, layout_root_key);
            layout_root_entry.layout(scaffold.body);
            layout_root_entry.column(14.0f, 16.0f);
            layout_root_entry.main_axis_tracks({
                guinevere::ui::Flex(1.0f).min(220.0f).pref(220.0f).priority(1),
                guinevere::ui::Fixed(footer_height).priority(2)
            });
            layout_root_entry.align_stretch();
            layout_root_entry.justify_start();

            app_component.mount_component(
                app_component.auto_local_key("editor"),
                layout_root_key,
                text_editor
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
                    .min_height = 460.0f,
                    .max_height = viewport.h - 20.0f,
                    .fill_viewport_height_below = 120.0f
                },
                .header_height = guinevere::ui::ResponsiveScalar{34.0f, 36.0f, 38.0f},
                .header_gap = guinevere::ui::ResponsiveScalar{10.0f, 12.0f, 14.0f}
            },
            "demo_textedit",
            render_frame,
            guinevere::gfx::Color{0.08f, 0.10f, 0.13f, 1.0f}
        );
    };

    const guinevere::app::ErrorCallback callbacks_error = [](std::string_view message) {
        std::cerr << "[guinevere::app::run] " << message << '\n';
    };

    return guinevere::app::run(config, callbacks, callbacks_error);
}
