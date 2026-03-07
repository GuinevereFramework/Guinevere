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
        && lhs.text_selection_anchor == rhs.text_selection_anchor
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

[[nodiscard]] std::size_t selection_start(std::size_t cursor, std::size_t anchor) noexcept
{
    return std::min(cursor, anchor);
}

[[nodiscard]] std::size_t selection_end(std::size_t cursor, std::size_t anchor) noexcept
{
    return std::max(cursor, anchor);
}

[[nodiscard]] bool has_selection(std::size_t cursor, std::size_t anchor) noexcept
{
    return cursor != anchor;
}

[[nodiscard]] bool erase_selected_text(
    std::string& text,
    std::size_t& cursor,
    std::size_t& anchor
)
{
    cursor = clamp_utf8_cursor(text, cursor);
    anchor = clamp_utf8_cursor(text, anchor);
    const std::size_t start = selection_start(cursor, anchor);
    const std::size_t end = selection_end(cursor, anchor);
    if(start == end) {
        anchor = cursor;
        return false;
    }

    text.erase(start, end - start);
    cursor = start;
    anchor = start;
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

constexpr float kTextEditPaddingX = 12.0f;
constexpr float kTextEditLineHeight = 24.0f;
constexpr float kTextEditLineTopOffset = 16.0f;

struct TextEditLine {
    std::size_t start = 0U;
    std::size_t end = 0U;
    float width = 0.0f;
};

[[nodiscard]] bool text_edit_is_multiline(const UiNode& node) noexcept
{
    return node.props.text_edit_input_type == TextEditInputType::MultiLine;
}

[[nodiscard]] std::size_t count_text_edit_newlines(std::string_view text) noexcept
{
    std::size_t count = 0U;
    for(const char ch : text) {
        if(ch == '\n') {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] std::size_t count_text_edit_lines(std::string_view text) noexcept
{
    return 1U + count_text_edit_newlines(text);
}

[[nodiscard]] std::vector<TextEditLine> build_text_edit_lines(
    std::string_view text,
    gfx::Renderer* renderer
)
{
    std::vector<TextEditLine> lines;
    std::size_t line_start = 0U;
    for(std::size_t i = 0U; i < text.size(); ++i) {
        if(text[i] != '\n') {
            continue;
        }

        const std::string_view line_text = text.substr(line_start, i - line_start);
        lines.push_back(TextEditLine{
            .start = line_start,
            .end = i,
            .width = measure_text_width(line_text, renderer)
        });
        line_start = i + 1U;
    }

    lines.push_back(TextEditLine{
        .start = line_start,
        .end = text.size(),
        .width = measure_text_width(text.substr(line_start), renderer)
    });
    return lines;
}

[[nodiscard]] std::size_t text_edit_line_index_for_cursor(
    const std::vector<TextEditLine>& lines,
    std::size_t cursor
) noexcept
{
    if(lines.empty()) {
        return 0U;
    }

    for(std::size_t i = 0U; i < lines.size(); ++i) {
        if(cursor >= lines[i].start && cursor <= lines[i].end) {
            return i;
        }
    }

    return lines.size() - 1U;
}

[[nodiscard]] std::size_t move_text_edit_cursor_vertically(
    std::string_view text,
    std::size_t cursor,
    int direction,
    gfx::Renderer* renderer
)
{
    const std::vector<TextEditLine> lines = build_text_edit_lines(text, renderer);
    if(lines.empty()) {
        return clamp_utf8_cursor(text, cursor);
    }

    const std::size_t clamped_cursor = clamp_utf8_cursor(text, cursor);
    const std::size_t current_line_index = text_edit_line_index_for_cursor(lines, clamped_cursor);
    if(direction < 0) {
        if(current_line_index == 0U) {
            return 0U;
        }
    } else if(current_line_index + 1U >= lines.size()) {
        return text.size();
    }

    const TextEditLine& current_line = lines[current_line_index];
    const std::string_view current_line_text(
        text.data() + current_line.start,
        current_line.end - current_line.start
    );
    const std::size_t current_line_cursor =
        std::min(clamped_cursor, current_line.end) - current_line.start;
    const float cursor_x = measure_text_width(
        current_line_text.substr(0U, current_line_cursor),
        renderer
    );

    const std::size_t target_line_index = direction < 0
        ? current_line_index - 1U
        : current_line_index + 1U;
    const TextEditLine& target_line = lines[target_line_index];
    const std::string_view target_line_text(
        text.data() + target_line.start,
        target_line.end - target_line.start
    );
    return target_line.start
        + cursor_from_click_x(target_line_text, cursor_x, target_line.width, renderer);
}

[[nodiscard]] std::string sanitize_text_edit_insert(
    std::string_view text,
    TextEditInputType input_type,
    std::size_t available_newlines
)
{
    std::string out;
    out.reserve(text.size());

    bool previous_was_carriage_return = false;
    for(const char ch : text) {
        if(ch == '\r') {
            if(input_type == TextEditInputType::SingleLine) {
                out.push_back(' ');
            } else if(available_newlines > 0U) {
                out.push_back('\n');
                --available_newlines;
            }
            previous_was_carriage_return = true;
            continue;
        }

        if(ch == '\n') {
            if(previous_was_carriage_return) {
                previous_was_carriage_return = false;
                continue;
            }
            if(input_type == TextEditInputType::SingleLine) {
                out.push_back(' ');
            } else if(available_newlines > 0U) {
                out.push_back('\n');
                --available_newlines;
            }
            continue;
        }

        previous_was_carriage_return = false;
        out.push_back(ch);
    }

    return out;
}

[[nodiscard]] bool insert_text_edit_text(
    const UiNode& node,
    std::string& value_utf8,
    std::size_t& cursor,
    std::size_t& anchor,
    std::string_view inserted_text
)
{
    cursor = clamp_utf8_cursor(value_utf8, cursor);
    anchor = clamp_utf8_cursor(value_utf8, anchor);

    const std::size_t selection_begin = selection_start(cursor, anchor);
    const std::size_t selection_limit = selection_end(cursor, anchor);

    std::size_t available_newlines = 0U;
    if(node.props.text_edit_input_type == TextEditInputType::MultiLine) {
        if(node.props.text_edit_max_lines == 0U) {
            available_newlines = static_cast<std::size_t>(-1);
        } else {
            const std::size_t current_line_count = count_text_edit_lines(value_utf8);
            const std::size_t selected_newline_count = count_text_edit_newlines(
                std::string_view(value_utf8).substr(
                    selection_begin,
                    selection_limit - selection_begin
                )
            );
            const std::size_t base_line_count =
                std::max<std::size_t>(1U, current_line_count - selected_newline_count);
            if(node.props.text_edit_max_lines > base_line_count) {
                available_newlines = node.props.text_edit_max_lines - base_line_count;
            }
        }
    }

    const std::string sanitized = sanitize_text_edit_insert(
        inserted_text,
        node.props.text_edit_input_type,
        available_newlines
    );
    if(sanitized.empty()) {
        return false;
    }

    value_utf8.erase(selection_begin, selection_limit - selection_begin);
    value_utf8.insert(selection_begin, sanitized);
    cursor = selection_begin + sanitized.size();
    cursor = clamp_utf8_cursor(value_utf8, cursor);
    anchor = cursor;
    return true;
}

[[nodiscard]] std::size_t text_edit_cursor_from_mouse(
    const UiNode& node,
    std::string_view value_utf8,
    const InputState& input,
    gfx::Renderer* renderer
)
{
    const float clip_x = node.layout.x + kTextEditPaddingX;
    const float clip_w = std::max(0.0f, node.layout.w - (2.0f * kTextEditPaddingX));
    if(clip_w <= 0.0f) {
        return clamp_utf8_cursor(value_utf8, node.state.text_cursor);
    }

    if(text_edit_is_multiline(node)) {
        const std::vector<TextEditLine> lines = build_text_edit_lines(value_utf8, renderer);
        if(lines.empty()) {
            return clamp_utf8_cursor(value_utf8, node.state.text_cursor);
        }

        std::size_t visible_lines = lines.size();
        if(node.props.text_edit_max_lines > 0U) {
            visible_lines = std::min(visible_lines, node.props.text_edit_max_lines);
        }
        visible_lines = std::max<std::size_t>(1U, visible_lines);

        const float local_y = input.y - (node.layout.y + kTextEditLineTopOffset);
        const float max_local_y = std::max(
            0.0f,
            (static_cast<float>(visible_lines) * kTextEditLineHeight) - 0.001f
        );
        const float clamped_local_y = std::clamp(local_y, 0.0f, max_local_y);
        const std::size_t line_index = std::min<std::size_t>(
            visible_lines - 1U,
            static_cast<std::size_t>(clamped_local_y / kTextEditLineHeight)
        );
        const TextEditLine& line = lines[line_index];
        const std::string_view line_text(value_utf8.data() + line.start, line.end - line.start);
        const float local_x = std::clamp(
            input.x - clip_x,
            0.0f,
            std::max(0.0f, line.width)
        );
        return line.start + cursor_from_click_x(line_text, local_x, line.width, renderer);
    }

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
        input.x - clip_x + scroll_x,
        0.0f,
        std::max(0.0f, text_width)
    );
    return cursor_from_click_x(value_utf8, local_x, text_width, renderer);
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
                node.state.text_cursor =
                    text_edit_cursor_from_mouse(node, value_utf8, input_, renderer);
            }
            node.state.text_selection_anchor = node.state.text_cursor;
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
    node.state.text_selection_anchor =
        clamp_utf8_cursor(value_utf8, node.state.text_selection_anchor);
    if(node.state.focused && !was_focused && !focused_by_mouse_click) {
        node.state.text_cursor = value_utf8.size();
        node.state.text_selection_anchor = node.state.text_cursor;
    }

    if(node.state.focused
       && input_.left_down
       && node.props.allow_mouse_set_cursor
       && node.props.allow_mouse_drag_selection
       && active_text_edit_key_ == node.key) {
        node.state.text_cursor = text_edit_cursor_from_mouse(node, value_utf8, input_, renderer);
        node.state.text_cursor = clamp_utf8_cursor(value_utf8, node.state.text_cursor);
    }

    bool changed = false;
    if(node.state.focused) {
        if(node.props.allow_user_toggle_caret && (input_.toggle_caret_presses % 2U) == 1U) {
            node.state.caret_visible = !node.state.caret_visible;
        }

        if(node.props.allow_ctrl_a && input_.ctrl_a_presses > 0U) {
            node.state.text_selection_anchor = 0U;
            node.state.text_cursor = value_utf8.size();
        }

        if(node.props.allow_ctrl_c && input_.ctrl_c_presses > 0U) {
            const std::size_t selection_begin = selection_start(
                node.state.text_cursor,
                node.state.text_selection_anchor
            );
            const std::size_t selection_limit = selection_end(
                node.state.text_cursor,
                node.state.text_selection_anchor
            );
            if(selection_begin != selection_limit) {
                clipboard_utf8_ =
                    value_utf8.substr(selection_begin, selection_limit - selection_begin);
            }
        }

        if(node.props.allow_ctrl_x && input_.ctrl_x_presses > 0U) {
            const std::size_t selection_begin = selection_start(
                node.state.text_cursor,
                node.state.text_selection_anchor
            );
            const std::size_t selection_limit = selection_end(
                node.state.text_cursor,
                node.state.text_selection_anchor
            );
            if(selection_begin != selection_limit) {
                clipboard_utf8_ =
                    value_utf8.substr(selection_begin, selection_limit - selection_begin);
                value_utf8.erase(selection_begin, selection_limit - selection_begin);
                node.state.text_cursor = selection_begin;
                node.state.text_selection_anchor = selection_begin;
                changed = true;
            }
        }

        if(node.props.allow_ctrl_v && input_.ctrl_v_presses > 0U && !clipboard_utf8_.empty()) {
            for(unsigned int i = 0U; i < input_.ctrl_v_presses; ++i) {
                if(insert_text_edit_text(
                       node,
                       value_utf8,
                       node.state.text_cursor,
                       node.state.text_selection_anchor,
                       clipboard_utf8_
                   )) {
                    changed = true;
                }
            }
        }

        if(node.props.allow_arrow_left) {
            for(unsigned int i = 0U; i < input_.left_arrow_presses; ++i) {
                if(has_selection(node.state.text_cursor, node.state.text_selection_anchor)) {
                    node.state.text_cursor = selection_start(
                        node.state.text_cursor,
                        node.state.text_selection_anchor
                    );
                } else {
                    node.state.text_cursor =
                        previous_utf8_boundary(value_utf8, node.state.text_cursor);
                }
                node.state.text_selection_anchor = node.state.text_cursor;
            }
        }

        if(node.props.allow_arrow_right) {
            for(unsigned int i = 0U; i < input_.right_arrow_presses; ++i) {
                if(has_selection(node.state.text_cursor, node.state.text_selection_anchor)) {
                    node.state.text_cursor = selection_end(
                        node.state.text_cursor,
                        node.state.text_selection_anchor
                    );
                } else {
                    node.state.text_cursor = next_utf8_boundary(value_utf8, node.state.text_cursor);
                }
                node.state.text_selection_anchor = node.state.text_cursor;
            }
        }

        if(node.props.allow_arrow_up && input_.up_arrow_presses > 0U) {
            for(unsigned int i = 0U; i < input_.up_arrow_presses; ++i) {
                if(text_edit_is_multiline(node)) {
                    if(has_selection(node.state.text_cursor, node.state.text_selection_anchor)) {
                        node.state.text_cursor = selection_start(
                            node.state.text_cursor,
                            node.state.text_selection_anchor
                        );
                    } else {
                        node.state.text_cursor = move_text_edit_cursor_vertically(
                            value_utf8,
                            node.state.text_cursor,
                            -1,
                            renderer
                        );
                    }
                } else {
                    node.state.text_cursor = 0U;
                }
                node.state.text_selection_anchor = node.state.text_cursor;
            }
        }

        if(node.props.allow_arrow_down && input_.down_arrow_presses > 0U) {
            for(unsigned int i = 0U; i < input_.down_arrow_presses; ++i) {
                if(text_edit_is_multiline(node)) {
                    if(has_selection(node.state.text_cursor, node.state.text_selection_anchor)) {
                        node.state.text_cursor = selection_end(
                            node.state.text_cursor,
                            node.state.text_selection_anchor
                        );
                    } else {
                        node.state.text_cursor = move_text_edit_cursor_vertically(
                            value_utf8,
                            node.state.text_cursor,
                            1,
                            renderer
                        );
                    }
                } else {
                    node.state.text_cursor = value_utf8.size();
                }
                node.state.text_selection_anchor = node.state.text_cursor;
            }
        }

        node.state.text_cursor = clamp_utf8_cursor(value_utf8, node.state.text_cursor);
        node.state.text_selection_anchor =
            clamp_utf8_cursor(value_utf8, node.state.text_selection_anchor);

        for(unsigned int i = 0U; i < input_.backspace_presses; ++i) {
            if(erase_selected_text(
                   value_utf8,
                   node.state.text_cursor,
                   node.state.text_selection_anchor
               )) {
                changed = true;
                continue;
            }

            if(!erase_previous_utf8_codepoint(value_utf8, node.state.text_cursor)) {
                break;
            }
            node.state.text_selection_anchor = node.state.text_cursor;
            changed = true;
        }

        if(!input_.text_utf8.empty()) {
            if(insert_text_edit_text(
                   node,
                   value_utf8,
                   node.state.text_cursor,
                   node.state.text_selection_anchor,
                   input_.text_utf8
               )) {
                changed = true;
            }
        }

        if(input_.enter_presses > 0U) {
            if(text_edit_is_multiline(node)) {
                const std::string newline_insert(input_.enter_presses, '\n');
                if(insert_text_edit_text(
                       node,
                       value_utf8,
                       node.state.text_cursor,
                       node.state.text_selection_anchor,
                       newline_insert
                   )) {
                    changed = true;
                }
            } else if(submitted != nullptr) {
                *submitted = true;
            }
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
        if(node.layout_config.width_mode == SizeMode::Auto
            || node.layout_config.height_mode == SizeMode::Auto) {
            node.dirty_flags |= DirtyFlags::Layout;
        }
    }

    return changed;
}

void InteractionState::end_frame() noexcept
{
    mouse_was_down_ = input_.left_down;
}

} // namespace guinevere::ui
