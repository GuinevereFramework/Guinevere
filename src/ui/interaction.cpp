#include <algorithm>

#include <guinevere/ui/runtime.hpp>

namespace guinevere::ui {

namespace {

[[nodiscard]] bool node_state_equal(const NodeState& lhs, const NodeState& rhs) noexcept
{
    return lhs.hovered == rhs.hovered
        && lhs.pressed == rhs.pressed
        && lhs.focused == rhs.focused
        && lhs.disabled == rhs.disabled
        && lhs.scroll_x == rhs.scroll_x
        && lhs.scroll_y == rhs.scroll_y
        && lhs.text_cursor == rhs.text_cursor
        && lhs.caret_visible == rhs.caret_visible;
}

[[nodiscard]] bool contains(float x, float y, gfx::Rect rect)
{
    return x >= rect.x
        && x <= (rect.x + rect.w)
        && y >= rect.y
        && y <= (rect.y + rect.h);
}

[[nodiscard]] bool is_utf8_continuation_byte(unsigned char byte)
{
    return (byte & 0xC0u) == 0x80u;
}

[[nodiscard]] std::size_t previous_utf8_boundary(std::string_view text, std::size_t cursor)
{
    if(cursor == 0U || text.empty()) {
        return 0U;
    }

    std::size_t i = std::min(cursor, text.size());
    do {
        --i;
    } while(i > 0U && is_utf8_continuation_byte(static_cast<unsigned char>(text[i])));

    return i;
}

[[nodiscard]] std::size_t next_utf8_boundary(std::string_view text, std::size_t cursor)
{
    if(text.empty()) {
        return 0U;
    }

    std::size_t i = std::min(cursor, text.size());
    if(i >= text.size()) {
        return text.size();
    }

    ++i;
    while(i < text.size() && is_utf8_continuation_byte(static_cast<unsigned char>(text[i]))) {
        ++i;
    }
    return i;
}

[[nodiscard]] std::size_t clamp_utf8_cursor(std::string_view text, std::size_t cursor)
{
    std::size_t clamped = std::min(cursor, text.size());
    if(clamped == text.size()) {
        return clamped;
    }

    while(clamped > 0U && is_utf8_continuation_byte(static_cast<unsigned char>(text[clamped]))) {
        --clamped;
    }
    return clamped;
}

[[nodiscard]] bool erase_previous_utf8_codepoint(std::string& text, std::size_t& cursor)
{
    cursor = clamp_utf8_cursor(text, cursor);
    if(cursor == 0U || text.empty()) {
        return false;
    }

    const std::size_t start = previous_utf8_boundary(text, cursor);
    text.erase(start, cursor - start);
    cursor = start;
    return true;
}

[[nodiscard]] std::size_t count_utf8_codepoints(std::string_view text)
{
    std::size_t count = 0U;
    for(std::size_t i = 0U; i < text.size();) {
        ++count;
        i = next_utf8_boundary(text, i);
    }
    return count;
}

[[nodiscard]] float measure_text_width(std::string_view text, gfx::Renderer* renderer)
{
    if(renderer != nullptr) {
        return renderer->measure_text(text);
    }

    constexpr float approx_glyph_width = 14.0f;
    return static_cast<float>(count_utf8_codepoints(text)) * approx_glyph_width;
}

[[nodiscard]] std::size_t approximate_cursor_from_ratio(std::string_view text, float ratio)
{
    const std::size_t codepoint_count = count_utf8_codepoints(text);
    if(codepoint_count == 0U) {
        return 0U;
    }

    const float clamped = std::clamp(ratio, 0.0f, 1.0f);
    const std::size_t target_codepoint = static_cast<std::size_t>(
        clamped * static_cast<float>(codepoint_count) + 0.5f
    );

    std::size_t idx = 0U;
    std::size_t consumed = 0U;
    while(consumed < target_codepoint && idx < text.size()) {
        idx = next_utf8_boundary(text, idx);
        ++consumed;
    }
    return idx;
}

[[nodiscard]] std::size_t cursor_from_click_x(
    std::string_view text,
    float click_x_in_text,
    float text_width,
    gfx::Renderer* renderer
)
{
    if(text.empty() || click_x_in_text <= 0.0f) {
        return 0U;
    }

    if(click_x_in_text >= text_width) {
        return text.size();
    }

    if(renderer == nullptr) {
        const float ratio = text_width > 0.0f ? (click_x_in_text / text_width) : 0.0f;
        return approximate_cursor_from_ratio(text, ratio);
    }

    std::size_t prev = 0U;
    float prev_w = 0.0f;
    while(prev < text.size()) {
        const std::size_t next = next_utf8_boundary(text, prev);
        const float next_w = renderer->measure_text(text.substr(0U, next));
        const float mid = prev_w + ((next_w - prev_w) * 0.5f);
        if(click_x_in_text <= mid) {
            return prev;
        }
        if(click_x_in_text <= next_w) {
            return next;
        }
        prev = next;
        prev_w = next_w;
    }

    return text.size();
}

struct ComputedSize {
    float width = 0.0f;
    float height = 0.0f;
};

} // namespace

void InteractionState::begin_frame(InputState input) noexcept
{
    input_ = input;
    pressed_edge_ = input_.left_down && !mouse_was_down_;
    released_edge_ = !input_.left_down && mouse_was_down_;
}

bool InteractionState::update_frame(
    UiTree& tree,
    const std::vector<ReconciledNode>& frame,
    gfx::Renderer* renderer
)
{
    bool any_handled = false;

    for(const ReconciledNode& entry : frame) {
        if(entry.node.key.empty()) {
            continue;
        }

        const std::size_t node_index = tree.find(entry.node.key);
        UiNode* node = tree.get(node_index);
        if(node == nullptr) {
            continue;
        }

        if(entry.node.kind == NodeKind::Button) {
            const bool clicked = update_button(*node);
            if(clicked && entry.node.on_click) {
                entry.node.on_click();
                any_handled = true;
            }
            continue;
        }

        if(entry.node.kind == NodeKind::TextEdit) {
            std::string value_utf8 = node->props.text;
            bool submitted = false;
            const bool changed = update_text_edit(*node, value_utf8, &submitted, renderer);
            if(changed && entry.node.on_text_change) {
                entry.node.on_text_change(value_utf8);
                any_handled = true;
            }
            if(submitted && entry.node.on_text_submit) {
                entry.node.on_text_submit(value_utf8);
                any_handled = true;
            }
        }
    }

    return any_handled;
}

bool InteractionState::update_scroll_container(UiNode& node, float pixels_per_scroll_step)
{
    if(node.layout_config.overflow != OverflowPolicy::Scroll) {
        return false;
    }

    const bool hovered = contains(input_.x, input_.y, node.layout);
    if(!hovered) {
        return false;
    }

    const float step = std::max(1.0f, pixels_per_scroll_step);

    float delta_x = 0.0f;
    float delta_y = 0.0f;
    const LayoutDirection direction = node.layout_config.direction == LayoutDirection::None
        ? LayoutDirection::Column
        : node.layout_config.direction;
    if(direction == LayoutDirection::Row) {
        delta_x = (input_.scroll_x != 0.0f) ? input_.scroll_x : input_.scroll_y;
    } else {
        delta_y = (input_.scroll_y != 0.0f) ? input_.scroll_y : input_.scroll_x;
    }

    if(delta_x == 0.0f && delta_y == 0.0f) {
        return false;
    }

    const float previous_scroll_x = node.state.scroll_x;
    const float previous_scroll_y = node.state.scroll_y;
    node.state.scroll_x = std::max(0.0f, node.state.scroll_x - (delta_x * step));
    node.state.scroll_y = std::max(0.0f, node.state.scroll_y - (delta_y * step));

    const bool changed = previous_scroll_x != node.state.scroll_x
        || previous_scroll_y != node.state.scroll_y;
    if(changed) {
        node.dirty_flags |= DirtyFlags::Layout | DirtyFlags::Paint | DirtyFlags::Interaction;
    }

    return changed;
}

bool InteractionState::update_button(UiNode& node)
{
    if(node.kind != NodeKind::Button) {
        return false;
    }

    const NodeState previous_state = node.state;
    const bool hovered = contains(input_.x, input_.y, node.layout);
    node.state.hovered = hovered;
    node.state.pressed = hovered && input_.left_down;

    bool& armed = press_armed_[node.key];
    if(pressed_edge_) {
        armed = hovered;
    }

    bool clicked = false;
    if(released_edge_) {
        clicked = armed && hovered;
        armed = false;
    }

    if(clicked) {
        active_text_edit_key_.clear();
    }

    if(!node_state_equal(previous_state, node.state)) {
        node.dirty_flags |= DirtyFlags::Paint | DirtyFlags::Interaction;
    }

    return clicked;
}

bool InteractionState::update_text_edit(
    UiNode& node,
    std::string& value_utf8,
    bool* submitted,
    gfx::Renderer* renderer
)
{
    if(node.kind != NodeKind::TextEdit) {
        return false;
    }

    const NodeState previous_state = node.state;
    const std::string previous_text = node.props.text;

    if(submitted != nullptr) {
        *submitted = false;
    }

    const bool was_focused = node.state.focused;
    const bool hovered = contains(input_.x, input_.y, node.layout);
    node.state.hovered = hovered;
    node.state.pressed = hovered && input_.left_down;

    if(pressed_edge_) {
        if(hovered) {
            active_text_edit_key_ = node.key;
            if(node.props.allow_mouse_set_cursor) {
                constexpr float padding_x = 12.0f;
                const float clip_x = node.layout.x + padding_x;
                const float clip_w = std::max(0.0f, node.layout.w - (2.0f * padding_x));
                if(clip_w > 0.0f) {
                    const std::size_t cursor_index = clamp_utf8_cursor(value_utf8, node.state.text_cursor);
                    const float text_width = measure_text_width(value_utf8, renderer);
                    const float before_cursor_w = measure_text_width(
                        std::string_view(value_utf8).substr(0U, cursor_index),
                        renderer
                    );
                    const float max_scroll = std::max(0.0f, text_width - clip_w);
                    const float desired_scroll = std::max(
                        0.0f,
                        before_cursor_w - std::max(0.0f, clip_w - 4.0f)
                    );
                    const float scroll_x = std::clamp(desired_scroll, 0.0f, max_scroll);
                    const float local_x = std::clamp(
                        input_.x - clip_x + scroll_x,
                        0.0f,
                        std::max(0.0f, text_width)
                    );
                    node.state.text_cursor = cursor_from_click_x(
                        value_utf8,
                        local_x,
                        text_width,
                        renderer
                    );
                }
            }
        } else if(active_text_edit_key_ == node.key) {
            active_text_edit_key_.clear();
        }
    }

    node.state.focused = (active_text_edit_key_ == node.key);
    const bool focused_by_mouse_click = pressed_edge_
        && hovered
        && node.state.focused
        && node.props.allow_mouse_set_cursor;
    node.state.text_cursor = clamp_utf8_cursor(value_utf8, node.state.text_cursor);
    if(node.state.focused && !was_focused && !focused_by_mouse_click) {
        node.state.text_cursor = value_utf8.size();
    }

    bool changed = false;
    if(node.state.focused) {
        if(node.props.allow_user_toggle_caret && (input_.toggle_caret_presses % 2U) == 1U) {
            node.state.caret_visible = !node.state.caret_visible;
        }

        if(node.props.allow_arrow_left) {
            for(unsigned int i = 0U; i < input_.left_arrow_presses; ++i) {
                node.state.text_cursor = previous_utf8_boundary(value_utf8, node.state.text_cursor);
            }
        }

        if(node.props.allow_arrow_right) {
            for(unsigned int i = 0U; i < input_.right_arrow_presses; ++i) {
                node.state.text_cursor = next_utf8_boundary(value_utf8, node.state.text_cursor);
            }
        }

        if(node.props.allow_arrow_up && input_.up_arrow_presses > 0U) {
            node.state.text_cursor = 0U;
        }

        if(node.props.allow_arrow_down && input_.down_arrow_presses > 0U) {
            node.state.text_cursor = value_utf8.size();
        }

        node.state.text_cursor = clamp_utf8_cursor(value_utf8, node.state.text_cursor);

        for(unsigned int i = 0U; i < input_.backspace_presses; ++i) {
            if(!erase_previous_utf8_codepoint(value_utf8, node.state.text_cursor)) {
                break;
            }
            changed = true;
        }

        if(!input_.text_utf8.empty()) {
            value_utf8.insert(node.state.text_cursor, input_.text_utf8);
            node.state.text_cursor += input_.text_utf8.size();
            node.state.text_cursor = clamp_utf8_cursor(value_utf8, node.state.text_cursor);
            changed = true;
        }

        if(input_.enter_presses > 0U && submitted != nullptr) {
            *submitted = true;
        }
    }

    if(node.props.text != value_utf8) {
        node.props.text = value_utf8;
    }

    if(!node_state_equal(previous_state, node.state) || previous_text != node.props.text) {
        node.dirty_flags |= DirtyFlags::Paint | DirtyFlags::Interaction;
    }
    if(previous_text != node.props.text) {
        node.dirty_flags |= DirtyFlags::Resource;
    }

    return changed;
}

void InteractionState::end_frame() noexcept
{
    mouse_was_down_ = input_.left_down;
}

} // namespace guinevere::ui
