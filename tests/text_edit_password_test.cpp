#include <string>

#include <guinevere/guinevere.hpp>

namespace {

bool focus_text_edit(
    guinevere::ui::InteractionState& interactions,
    guinevere::ui::UiNode& node,
    std::string& value_utf8
)
{
    interactions.begin_frame(guinevere::ui::InputState{
        .x = 4.0f,
        .y = 4.0f,
        .left_down = true
    });
    const bool changed = interactions.update_text_edit(node, value_utf8);
    interactions.end_frame();

    interactions.begin_frame(guinevere::ui::InputState{});
    interactions.update_text_edit(node, value_utf8);
    interactions.end_frame();
    return changed;
}

} // namespace

int main()
{
    {
        guinevere::ui::InteractionState interactions;
        guinevere::ui::UiNode node;
        node.key = "password";
        node.kind = guinevere::ui::NodeKind::TextEdit;
        node.layout = guinevere::gfx::Rect{0.0f, 0.0f, 240.0f, 44.0f};
        node.props.text_edit_input_type = guinevere::ui::TextEditInputType::PasswordLine;
        node.props.text_edit_echo_mode = guinevere::ui::TextEditEchoMode::password();

        std::string value_utf8 = "secret";
        focus_text_edit(interactions, node, value_utf8);
        node.state.text_selection_anchor = 0U;
        node.state.text_cursor = value_utf8.size();

        interactions.begin_frame(guinevere::ui::InputState{.ctrl_c_presses = 1U});
        interactions.update_text_edit(node, value_utf8);
        interactions.end_frame();

        node.state.text_selection_anchor = node.state.text_cursor = value_utf8.size();
        interactions.begin_frame(guinevere::ui::InputState{.ctrl_v_presses = 1U});
        if(interactions.update_text_edit(node, value_utf8)) {
            return 1;
        }
        interactions.end_frame();
        if(value_utf8 != "secret") {
            return 1;
        }
    }

    {
        guinevere::ui::InteractionState interactions;
        guinevere::ui::UiNode node;
        node.key = "password";
        node.kind = guinevere::ui::NodeKind::TextEdit;
        node.layout = guinevere::gfx::Rect{0.0f, 0.0f, 240.0f, 44.0f};
        node.props.text_edit_input_type = guinevere::ui::TextEditInputType::PasswordLine;
        node.props.text_edit_echo_mode = guinevere::ui::TextEditEchoMode::show_in(250U);

        std::string value_utf8;
        focus_text_edit(interactions, node, value_utf8);

        interactions.begin_frame(guinevere::ui::InputState{
            .elapsed_seconds = 1.0,
            .text_utf8 = "abc"
        });
        if(!interactions.update_text_edit(node, value_utf8)) {
            return 1;
        }
        interactions.end_frame();

        if(value_utf8 != "abc") {
            return 1;
        }
        if(node.state.text_reveal_begin != 0U || node.state.text_reveal_end != value_utf8.size()) {
            return 1;
        }
        if(node.state.text_reveal_until_seconds < 1.24 || node.state.text_reveal_until_seconds > 1.26) {
            return 1;
        }

        node.state.text_selection_anchor = 0U;
        node.state.text_cursor = value_utf8.size();
        interactions.begin_frame(guinevere::ui::InputState{
            .elapsed_seconds = 1.3,
            .ctrl_x_presses = 1U
        });
        if(!interactions.update_text_edit(node, value_utf8)) {
            return 1;
        }
        interactions.end_frame();

        if(!value_utf8.empty()) {
            return 1;
        }

        interactions.begin_frame(guinevere::ui::InputState{
            .elapsed_seconds = 1.4,
            .ctrl_v_presses = 1U
        });
        if(interactions.update_text_edit(node, value_utf8)) {
            return 1;
        }
        interactions.end_frame();
        if(!value_utf8.empty()) {
            return 1;
        }
    }

    return 0;
}
