#include <algorithm>
#include <numeric>

#include <guinevere/asset/asset_manager.hpp>
#include <guinevere/ui/runtime.hpp>

namespace guinevere::ui {

namespace {

[[nodiscard]] bool rect_is_empty(gfx::Rect rect) noexcept
{
    return rect.w <= 0.0f || rect.h <= 0.0f;
}

[[nodiscard]] bool rect_equal(gfx::Rect lhs, gfx::Rect rhs) noexcept
{
    return lhs.x == rhs.x
        && lhs.y == rhs.y
        && lhs.w == rhs.w
        && lhs.h == rhs.h;
}

[[nodiscard]] bool rect_intersects(gfx::Rect lhs, gfx::Rect rhs) noexcept
{
    const float lhs_right = lhs.x + lhs.w;
    const float lhs_bottom = lhs.y + lhs.h;
    const float rhs_right = rhs.x + rhs.w;
    const float rhs_bottom = rhs.y + rhs.h;
    return lhs.x < rhs_right
        && lhs_right > rhs.x
        && lhs.y < rhs_bottom
        && lhs_bottom > rhs.y;
}

[[nodiscard]] gfx::Rect rect_union(gfx::Rect lhs, gfx::Rect rhs) noexcept
{
    if(rect_is_empty(lhs)) {
        return rhs;
    }
    if(rect_is_empty(rhs)) {
        return lhs;
    }

    const float x0 = std::min(lhs.x, rhs.x);
    const float y0 = std::min(lhs.y, rhs.y);
    const float x1 = std::max(lhs.x + lhs.w, rhs.x + rhs.w);
    const float y1 = std::max(lhs.y + lhs.h, rhs.y + rhs.h);
    return gfx::Rect{x0, y0, std::max(0.0f, x1 - x0), std::max(0.0f, y1 - y0)};
}

void assign_layout(UiTree& tree, std::size_t node_index, gfx::Rect layout)
{
    UiNode* node = tree.get(node_index);
    if(node == nullptr) {
        return;
    }

    if(rect_equal(node->layout, layout)) {
        return;
    }

    const gfx::Rect old_layout = node->layout;
    node->previous_layout = old_layout;
    node->layout = layout;
    node->dirty_flags |= DirtyFlags::Paint;
    tree.invalidate(rect_union(old_layout, layout));
}

[[nodiscard]] float intrinsic_height(NodeKind kind)
{
    switch(kind) {
    case NodeKind::Root:
        return 0.0f;
    case NodeKind::Panel:
        return 120.0f;
    case NodeKind::View:
        return 40.0f;
    case NodeKind::Image:
        return 180.0f;
    case NodeKind::Button:
        return 56.0f;
    case NodeKind::TextEdit:
        return 56.0f;
    case NodeKind::Label:
        return 32.0f;
    case NodeKind::Custom:
        return 48.0f;
    }

    return 48.0f;
}

[[nodiscard]] bool is_utf8_continuation_byte(unsigned char byte)
{
    return (byte & 0xC0u) == 0x80u;
}

[[nodiscard]] std::size_t count_utf8_codepoints(std::string_view text)
{
    std::size_t count = 0U;
    for(std::size_t i = 0U; i < text.size();) {
        ++count;
        ++i;
        while(i < text.size()
            && is_utf8_continuation_byte(static_cast<unsigned char>(text[i]))) {
            ++i;
        }
    }
    return count;
}

[[nodiscard]] float measure_text_width(std::string_view text, gfx::Renderer* renderer)
{
    if(renderer != nullptr) {
        return std::max(0.0f, renderer->measure_text(text));
    }

    constexpr float approx_glyph_width = 9.0f;
    return static_cast<float>(count_utf8_codepoints(text)) * approx_glyph_width;
}

[[nodiscard]] float intrinsic_width(const UiNode& child, float available_width, gfx::Renderer* renderer)
{
    switch(child.kind) {
    case NodeKind::Button:
        return std::max(112.0f, measure_text_width(child.props.text, renderer) + 32.0f);
    case NodeKind::TextEdit:
        return std::max(180.0f, measure_text_width(child.props.text, renderer) + 28.0f);
    case NodeKind::Label:
        return std::max(24.0f, measure_text_width(child.props.text, renderer));
    case NodeKind::Panel:
    case NodeKind::View:
    case NodeKind::Image:
    case NodeKind::Root:
        return available_width;
    case NodeKind::Custom:
        return std::min(available_width, 120.0f);
    }

    return available_width;
}

[[nodiscard]] float clamp_width_to_constraints(const UiNode& child, float width) noexcept
{
    float out = std::max(0.0f, width);
    const float min_width = std::max(0.0f, child.layout_config.min_width);
    float max_width = child.layout_config.max_width;
    if(max_width > 0.0f) {
        max_width = std::max(0.0f, max_width);
        if(max_width < min_width) {
            max_width = min_width;
        }
        out = std::clamp(out, min_width, max_width);
        return out;
    }
    return std::max(min_width, out);
}

[[nodiscard]] float clamp_height_to_constraints(const UiNode& child, float height) noexcept
{
    float out = std::max(0.0f, height);
    const float min_height = std::max(0.0f, child.layout_config.min_height);
    float max_height = child.layout_config.max_height;
    if(max_height > 0.0f) {
        max_height = std::max(0.0f, max_height);
        if(max_height < min_height) {
            max_height = min_height;
        }
        out = std::clamp(out, min_height, max_height);
        return out;
    }
    return std::max(min_height, out);
}

[[nodiscard]] float resolve_width(
    const UiNode& child,
    float available_width,
    float distributed_fill_width,
    gfx::Renderer* renderer
)
{
    switch(child.layout_config.width_mode) {
    case SizeMode::Fixed:
        return clamp_width_to_constraints(child, child.layout_config.fixed_width);
    case SizeMode::Fill:
        return clamp_width_to_constraints(child, distributed_fill_width);
    case SizeMode::Auto:
        return clamp_width_to_constraints(
            child,
            intrinsic_width(child, available_width, renderer)
        );
    }

    return clamp_width_to_constraints(child, available_width);
}

[[nodiscard]] float resolve_height(
    const UiNode& child,
    float available_height,
    float distributed_fill_height
)
{
    switch(child.layout_config.height_mode) {
    case SizeMode::Fixed:
        return clamp_height_to_constraints(child, child.layout_config.fixed_height);
    case SizeMode::Fill:
        return clamp_height_to_constraints(child, distributed_fill_height);
    case SizeMode::Auto:
        return clamp_height_to_constraints(
            child,
            std::min(intrinsic_height(child.kind), available_height)
        );
    }

    return clamp_height_to_constraints(child, available_height);
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

struct ComputedSize {
    float width = 0.0f;
    float height = 0.0f;
};


[[nodiscard]] float apply_cross_align_offset(
    AlignItems align,
    float container_size,
    float item_size
)
{
    const float remain = std::max(0.0f, container_size - item_size);
    switch(align) {
    case AlignItems::Start:
        return 0.0f;
    case AlignItems::Center:
        return remain * 0.5f;
    case AlignItems::End:
        return remain;
    case AlignItems::Stretch:
        return 0.0f;
    }

    return 0.0f;
}

[[nodiscard]] float apply_justify_offset(
    JustifyContent justify,
    float container_size,
    float item_size
)
{
    const float remain = std::max(0.0f, container_size - item_size);
    switch(justify) {
    case JustifyContent::Start:
    case JustifyContent::SpaceBetween:
        return 0.0f;
    case JustifyContent::Center:
        return remain * 0.5f;
    case JustifyContent::End:
        return remain;
    }

    return 0.0f;
}

struct MainAxisPlacement {
    float start = 0.0f;
    float gap = 0.0f;
};

[[nodiscard]] MainAxisPlacement resolve_main_axis_placement(
    JustifyContent justify,
    float axis_origin,
    float axis_size,
    float total_item_size,
    float configured_gap,
    std::size_t item_count
)
{
    MainAxisPlacement placement{};
    if(item_count == 0U) {
        placement.start = axis_origin;
        placement.gap = configured_gap;
        return placement;
    }

    const float default_gap_total = configured_gap * static_cast<float>(item_count - 1U);
    const float free_with_default_gap = std::max(0.0f, axis_size - total_item_size - default_gap_total);

    placement.start = axis_origin;
    placement.gap = configured_gap;

    switch(justify) {
    case JustifyContent::Start:
        break;
    case JustifyContent::Center:
        placement.start = axis_origin + (free_with_default_gap * 0.5f);
        break;
    case JustifyContent::End:
        placement.start = axis_origin + free_with_default_gap;
        break;
    case JustifyContent::SpaceBetween:
        if(item_count > 1U) {
            placement.gap = std::max(
                0.0f,
                (axis_size - total_item_size) / static_cast<float>(item_count - 1U)
            );
        }
        break;
    }

    return placement;
}

void layout_subtree(UiTree& tree, std::size_t node_index, gfx::Renderer* renderer)
{
    UiNode* node = tree.get(node_index);
    if(node == nullptr || node->children.empty()) {
        return;
    }

    const float padding = std::max(0.0f, node->layout_config.padding);
    const float gap = std::max(0.0f, node->layout_config.gap);
    const float inner_x = node->layout.x + padding;
    const float inner_y = node->layout.y + padding;
    const float inner_w = std::max(0.0f, node->layout.w - (2.0f * padding));
    const float inner_h = std::max(0.0f, node->layout.h - (2.0f * padding));
    const LayoutDirection direction = node->layout_config.direction == LayoutDirection::None
        ? LayoutDirection::Column
        : node->layout_config.direction;

    std::vector<std::size_t> auto_children;
    auto_children.reserve(node->children.size());
    for(const std::size_t child_index : node->children) {
        const UiNode* child = tree.get(child_index);
        if(child == nullptr || child->has_explicit_layout) {
            continue;
        }
        auto_children.push_back(child_index);
    }

    std::vector<ComputedSize> sizes;
    sizes.resize(auto_children.size());

    if(node->layout_config.overflow == OverflowPolicy::Wrap
        && direction != LayoutDirection::Grid) {
        if(direction == LayoutDirection::Row) {
            float cursor_x = inner_x;
            float cursor_y = inner_y;
            float line_height = 0.0f;
            const float line_end_x = inner_x + inner_w;

            for(const std::size_t child_index : auto_children) {
                UiNode* child = tree.get(child_index);
                if(child == nullptr) {
                    continue;
                }

                float width = 0.0f;
                if(child->layout_config.width_mode == SizeMode::Fill) {
                    const float remaining_line_w = std::max(0.0f, line_end_x - cursor_x);
                    width = resolve_width(*child, inner_w, remaining_line_w, renderer);
                } else {
                    width = resolve_width(*child, inner_w, inner_w, renderer);
                }
                float height = resolve_height(*child, inner_h, inner_h);
                if(node->layout_config.align_items == AlignItems::Stretch
                    && child->layout_config.height_mode == SizeMode::Auto) {
                    height = clamp_height_to_constraints(*child, inner_h);
                }

                const bool needs_wrap = (cursor_x > inner_x) && ((cursor_x + width) > line_end_x);
                if(needs_wrap) {
                    cursor_x = inner_x;
                    cursor_y += line_height + gap;
                    line_height = 0.0f;

                    if(child->layout_config.width_mode == SizeMode::Fill) {
                        const float remaining_line_w = std::max(0.0f, line_end_x - cursor_x);
                        width = resolve_width(*child, inner_w, remaining_line_w, renderer);
                    }
                }

                assign_layout(
                    tree,
                    child_index,
                    gfx::Rect{
                        cursor_x,
                        cursor_y,
                        width,
                        height
                    }
                );

                cursor_x += width + gap;
                line_height = std::max(line_height, height);
            }
        } else {
            float cursor_x = inner_x;
            float cursor_y = inner_y;
            float column_width = 0.0f;
            const float column_end_y = inner_y + inner_h;

            for(const std::size_t child_index : auto_children) {
                UiNode* child = tree.get(child_index);
                if(child == nullptr) {
                    continue;
                }

                float width = resolve_width(*child, inner_w, inner_w, renderer);
                if(node->layout_config.align_items == AlignItems::Stretch
                    && child->layout_config.width_mode == SizeMode::Auto) {
                    width = clamp_width_to_constraints(*child, inner_w);
                }
                float height = 0.0f;
                if(child->layout_config.height_mode == SizeMode::Fill) {
                    const float remaining_column_h = std::max(0.0f, column_end_y - cursor_y);
                    height = resolve_height(*child, inner_h, remaining_column_h);
                } else {
                    height = resolve_height(*child, inner_h, inner_h);
                }

                const bool needs_wrap = (cursor_y > inner_y) && ((cursor_y + height) > column_end_y);
                if(needs_wrap) {
                    cursor_y = inner_y;
                    cursor_x += column_width + gap;
                    column_width = 0.0f;

                    if(child->layout_config.height_mode == SizeMode::Fill) {
                        const float remaining_column_h = std::max(0.0f, column_end_y - cursor_y);
                        height = resolve_height(*child, inner_h, remaining_column_h);
                    }
                }

                assign_layout(
                    tree,
                    child_index,
                    gfx::Rect{
                        cursor_x,
                        cursor_y,
                        width,
                        height
                    }
                );

                cursor_y += height + gap;
                column_width = std::max(column_width, width);
            }
        }

        for(const std::size_t child_index : node->children) {
            layout_subtree(tree, child_index, renderer);
        }
        return;
    }

    if(direction == LayoutDirection::Grid) {
        if(auto_children.empty()) {
            for(const std::size_t child_index : node->children) {
                layout_subtree(tree, child_index, renderer);
            }
            return;
        }

        const std::size_t column_count = !node->layout_config.grid_columns.empty()
            ? node->layout_config.grid_columns.size()
            : std::max<std::size_t>(1U, node->layout_config.grid_auto_columns);
        const std::size_t row_count =
            (auto_children.size() + column_count - 1U) / column_count;

        std::vector<float> column_widths;
        if(!node->layout_config.grid_columns.empty()) {
            column_widths = resolve_axis_tracks(
                inner_w,
                node->layout_config.grid_columns,
                0.0f,
                gap
            );
        } else {
            const float total_gap = column_count > 1U
                ? gap * static_cast<float>(column_count - 1U)
                : 0.0f;
            const float available_columns_w = std::max(0.0f, inner_w - total_gap);
            const float column_w = column_count > 0U
                ? (available_columns_w / static_cast<float>(column_count))
                : 0.0f;
            column_widths.assign(column_count, column_w);
        }
        if(column_widths.size() < column_count) {
            column_widths.resize(column_count, 0.0f);
        }

        std::vector<float> row_heights(row_count, 0.0f);
        if(node->layout_config.grid_rows.size() == row_count) {
            row_heights = resolve_axis_tracks(
                inner_h,
                node->layout_config.grid_rows,
                0.0f,
                gap
            );
        } else {
            for(std::size_t i = 0U; i < auto_children.size(); ++i) {
                UiNode* child = tree.get(auto_children[i]);
                if(child == nullptr) {
                    continue;
                }

                const std::size_t row = i / column_count;
                const float child_h = resolve_height(*child, inner_h, inner_h);
                row_heights[row] = std::max(row_heights[row], child_h);
            }
        }

        const float total_grid_h = row_heights.empty()
            ? 0.0f
            : std::accumulate(row_heights.begin(), row_heights.end(), 0.0f)
                + (gap * static_cast<float>(row_heights.size() - 1U));
        if(node->layout_config.overflow == OverflowPolicy::Scroll) {
            const float max_scroll_y = std::max(0.0f, total_grid_h - inner_h);
            node->state.scroll_y = std::clamp(node->state.scroll_y, 0.0f, max_scroll_y);
            node->state.scroll_x = 0.0f;
        } else {
            node->state.scroll_x = 0.0f;
            node->state.scroll_y = 0.0f;
        }

        std::vector<float> column_x(column_count, inner_x);
        for(std::size_t col = 1U; col < column_count; ++col) {
            column_x[col] = column_x[col - 1U] + column_widths[col - 1U] + gap;
        }

        std::vector<float> row_y(row_count, inner_y - node->state.scroll_y);
        for(std::size_t row = 1U; row < row_count; ++row) {
            row_y[row] = row_y[row - 1U] + row_heights[row - 1U] + gap;
        }

        for(std::size_t i = 0U; i < auto_children.size(); ++i) {
            UiNode* child = tree.get(auto_children[i]);
            if(child == nullptr) {
                continue;
            }

            const std::size_t row = i / column_count;
            const std::size_t col = i % column_count;
            const float cell_x = column_x[col];
            const float cell_y = row_y[row];
            const float cell_w = column_widths[col];
            const float cell_h = row_heights[row];

            float child_w = resolve_width(*child, cell_w, cell_w, renderer);
            float child_h = resolve_height(*child, cell_h, cell_h);
            if(node->layout_config.align_items == AlignItems::Stretch
                && child->layout_config.width_mode == SizeMode::Auto) {
                child_w = clamp_width_to_constraints(*child, cell_w);
            }

            const float x_offset = apply_cross_align_offset(
                node->layout_config.align_items,
                cell_w,
                child_w
            );
            const float y_offset = apply_justify_offset(
                node->layout_config.justify_content,
                cell_h,
                child_h
            );

            assign_layout(
                tree,
                auto_children[i],
                gfx::Rect{
                    cell_x + x_offset,
                    cell_y + y_offset,
                    child_w,
                    child_h
                }
            );
        }

        for(const std::size_t child_index : node->children) {
            layout_subtree(tree, child_index, renderer);
        }
        return;
    }

    if(direction == LayoutDirection::Row) {
        const float total_gap = auto_children.empty()
            ? 0.0f
            : gap * static_cast<float>(auto_children.size() - 1U);
        const bool use_main_axis_tracks =
            node->layout_config.main_axis_tracks.size() == auto_children.size();
        float total_width = 0.0f;

        if(use_main_axis_tracks) {
            const std::vector<float> track_widths = resolve_axis_tracks(
                inner_w,
                node->layout_config.main_axis_tracks,
                0.0f,
                gap
            );

            for(std::size_t i = 0; i < auto_children.size(); ++i) {
                UiNode* child = tree.get(auto_children[i]);
                if(child == nullptr) {
                    continue;
                }

                sizes[i].width = clamp_width_to_constraints(*child, track_widths[i]);
                sizes[i].height = resolve_height(*child, inner_h, inner_h);
                if(node->layout_config.align_items == AlignItems::Stretch
                    && child->layout_config.height_mode == SizeMode::Auto) {
                    sizes[i].height = clamp_height_to_constraints(*child, inner_h);
                }
                total_width += sizes[i].width;
            }
        } else {
            float non_fill_width_sum = 0.0f;
            float fill_width_weight_sum = 0.0f;
            for(std::size_t i = 0; i < auto_children.size(); ++i) {
                const UiNode* child = tree.get(auto_children[i]);
                if(child == nullptr) {
                    continue;
                }

                if(child->layout_config.width_mode == SizeMode::Fill) {
                    fill_width_weight_sum += std::max(0.0f, child->layout_config.width_fill_weight);
                } else {
                    const float width = resolve_width(*child, inner_w, inner_w, renderer);
                    sizes[i].width = width;
                    non_fill_width_sum += width;
                }
            }

            const float remain_width = std::max(0.0f, inner_w - non_fill_width_sum - total_gap);
            for(std::size_t i = 0; i < auto_children.size(); ++i) {
                UiNode* child = tree.get(auto_children[i]);
                if(child == nullptr) {
                    continue;
                }

                if(child->layout_config.width_mode == SizeMode::Fill) {
                    const float weight = std::max(0.0f, child->layout_config.width_fill_weight);
                    const float fill_width = fill_width_weight_sum > 0.0f
                        ? remain_width * (weight / fill_width_weight_sum)
                        : 0.0f;
                    sizes[i].width = resolve_width(*child, inner_w, fill_width, renderer);
                }

                sizes[i].height = resolve_height(*child, inner_h, inner_h);
                if(node->layout_config.align_items == AlignItems::Stretch
                    && child->layout_config.height_mode == SizeMode::Auto) {
                    sizes[i].height = clamp_height_to_constraints(*child, inner_h);
                }
                total_width += sizes[i].width;
            }
        }

        if(node->layout_config.overflow == OverflowPolicy::Scroll) {
            const float total_content_width = total_width + total_gap;
            const float max_scroll_x = std::max(0.0f, total_content_width - inner_w);
            node->state.scroll_x = std::clamp(node->state.scroll_x, 0.0f, max_scroll_x);
            node->state.scroll_y = 0.0f;

            float cursor_x = inner_x - node->state.scroll_x;
            for(std::size_t i = 0; i < auto_children.size(); ++i) {
                UiNode* child = tree.get(auto_children[i]);
                if(child == nullptr) {
                    continue;
                }

                const float y_offset = apply_cross_align_offset(
                    node->layout_config.align_items,
                    inner_h,
                    sizes[i].height
                );
                assign_layout(
                    tree,
                    auto_children[i],
                    gfx::Rect{
                        cursor_x,
                        inner_y + y_offset,
                        sizes[i].width,
                        sizes[i].height
                    }
                );
                cursor_x += sizes[i].width + gap;
            }
        } else {
            const MainAxisPlacement placement = resolve_main_axis_placement(
                node->layout_config.justify_content,
                inner_x,
                inner_w,
                total_width,
                gap,
                auto_children.size()
            );

            float cursor_x = placement.start;
            for(std::size_t i = 0; i < auto_children.size(); ++i) {
                UiNode* child = tree.get(auto_children[i]);
                if(child == nullptr) {
                    continue;
                }

                const float y_offset = apply_cross_align_offset(
                    node->layout_config.align_items,
                    inner_h,
                    sizes[i].height
                );
                assign_layout(
                    tree,
                    auto_children[i],
                    gfx::Rect{
                        cursor_x,
                        inner_y + y_offset,
                        sizes[i].width,
                        sizes[i].height
                    }
                );
                cursor_x += sizes[i].width + placement.gap;
            }
        }
    } else {
        const float total_gap = auto_children.empty()
            ? 0.0f
            : gap * static_cast<float>(auto_children.size() - 1U);
        const bool use_main_axis_tracks =
            node->layout_config.main_axis_tracks.size() == auto_children.size();
        float total_height = 0.0f;

        if(use_main_axis_tracks) {
            const std::vector<float> track_heights = resolve_axis_tracks(
                inner_h,
                node->layout_config.main_axis_tracks,
                0.0f,
                gap
            );

            for(std::size_t i = 0; i < auto_children.size(); ++i) {
                UiNode* child = tree.get(auto_children[i]);
                if(child == nullptr) {
                    continue;
                }

                sizes[i].height = clamp_height_to_constraints(*child, track_heights[i]);
                sizes[i].width = resolve_width(*child, inner_w, inner_w, renderer);
                if(node->layout_config.align_items == AlignItems::Stretch
                    && child->layout_config.width_mode == SizeMode::Auto) {
                    sizes[i].width = clamp_width_to_constraints(*child, inner_w);
                }
                total_height += sizes[i].height;
            }
        } else {
            float non_fill_height_sum = 0.0f;
            float fill_height_weight_sum = 0.0f;
            for(std::size_t i = 0; i < auto_children.size(); ++i) {
                const UiNode* child = tree.get(auto_children[i]);
                if(child == nullptr) {
                    continue;
                }

                if(child->layout_config.height_mode == SizeMode::Fill) {
                    fill_height_weight_sum += std::max(0.0f, child->layout_config.height_fill_weight);
                } else {
                    const float height = resolve_height(*child, inner_h, inner_h);
                    sizes[i].height = height;
                    non_fill_height_sum += height;
                }
            }

            const float remain_height = std::max(0.0f, inner_h - non_fill_height_sum - total_gap);
            for(std::size_t i = 0; i < auto_children.size(); ++i) {
                UiNode* child = tree.get(auto_children[i]);
                if(child == nullptr) {
                    continue;
                }

                if(child->layout_config.height_mode == SizeMode::Fill) {
                    const float weight = std::max(0.0f, child->layout_config.height_fill_weight);
                    const float fill_height = fill_height_weight_sum > 0.0f
                        ? remain_height * (weight / fill_height_weight_sum)
                        : 0.0f;
                    sizes[i].height = resolve_height(*child, inner_h, fill_height);
                }

                sizes[i].width = resolve_width(*child, inner_w, inner_w, renderer);
                if(node->layout_config.align_items == AlignItems::Stretch
                    && child->layout_config.width_mode == SizeMode::Auto) {
                    sizes[i].width = clamp_width_to_constraints(*child, inner_w);
                }
                total_height += sizes[i].height;
            }
        }

        if(node->layout_config.overflow == OverflowPolicy::Scroll) {
            const float total_content_height = total_height + total_gap;
            const float max_scroll_y = std::max(0.0f, total_content_height - inner_h);
            node->state.scroll_y = std::clamp(node->state.scroll_y, 0.0f, max_scroll_y);
            node->state.scroll_x = 0.0f;

            float cursor_y = inner_y - node->state.scroll_y;
            for(std::size_t i = 0; i < auto_children.size(); ++i) {
                UiNode* child = tree.get(auto_children[i]);
                if(child == nullptr) {
                    continue;
                }

                const float x_offset = apply_cross_align_offset(
                    node->layout_config.align_items,
                    inner_w,
                    sizes[i].width
                );
                assign_layout(
                    tree,
                    auto_children[i],
                    gfx::Rect{
                        inner_x + x_offset,
                        cursor_y,
                        sizes[i].width,
                        sizes[i].height
                    }
                );
                cursor_y += sizes[i].height + gap;
            }
        } else {
            const MainAxisPlacement placement = resolve_main_axis_placement(
                node->layout_config.justify_content,
                inner_y,
                inner_h,
                total_height,
                gap,
                auto_children.size()
            );

            float cursor_y = placement.start;
            for(std::size_t i = 0; i < auto_children.size(); ++i) {
                UiNode* child = tree.get(auto_children[i]);
                if(child == nullptr) {
                    continue;
                }

                const float x_offset = apply_cross_align_offset(
                    node->layout_config.align_items,
                    inner_w,
                    sizes[i].width
                );
                assign_layout(
                    tree,
                    auto_children[i],
                    gfx::Rect{
                        inner_x + x_offset,
                        cursor_y,
                        sizes[i].width,
                        sizes[i].height
                    }
                );
                cursor_y += sizes[i].height + placement.gap;
            }
        }
    }

    for(const std::size_t child_index : node->children) {
        layout_subtree(tree, child_index, renderer);
    }
}

void emit_draw_commands(const UiTree& tree, std::size_t index, std::vector<DrawCommand>& out)
{
    const UiNode* node = tree.get(index);
    if(node == nullptr) {
        return;
    }

    const bool clips_children = node->kind == NodeKind::Panel
        && node->layout_config.overflow != OverflowPolicy::Visible;
    if(node->kind == NodeKind::Panel) {
        out.push_back(DrawCommand{
            DrawCommandType::FillRect,
            node->layout,
            gfx::Color{0.11f, 0.15f, 0.21f, 1.0f}
        });
        out.push_back(DrawCommand{
            DrawCommandType::StrokeRect,
            node->layout,
            gfx::Color{0.26f, 0.34f, 0.48f, 1.0f},
            2.0f
        });
        if(clips_children) {
            out.push_back(DrawCommand{
                DrawCommandType::PushClip,
                node->layout
            });
        }
    }

    if(node->kind == NodeKind::Label) {
        out.push_back(DrawCommand{
            DrawCommandType::Text,
            node->layout,
            gfx::Color{0.94f, 0.97f, 1.0f, 1.0f},
            1.0f,
            node->layout.x,
            node->layout.y + (node->layout.h * 0.62f),
            node->props.text
        });
    }

    if(node->kind == NodeKind::Image) {
        out.push_back(DrawCommand{
            DrawCommandType::FillRect,
            node->layout,
            gfx::Color{0.09f, 0.12f, 0.17f, 1.0f}
        });

        if(!node->props.image_source.empty()) {
            out.push_back(DrawCommand{
                .type = DrawCommandType::Image,
                .rect = node->layout,
                .color = node->props.image_tint,
                .image_source = node->props.image_source
            });
        }

        out.push_back(DrawCommand{
            DrawCommandType::StrokeRect,
            node->layout,
            gfx::Color{0.34f, 0.42f, 0.58f, 1.0f},
            1.0f
        });
    }

    if(node->kind == NodeKind::Button) {
        gfx::Color fill{0.18f, 0.24f, 0.34f, 1.0f};
        if(node->state.hovered) {
            fill = node->state.pressed
                ? gfx::Color{0.13f, 0.20f, 0.30f, 1.0f}
                : gfx::Color{0.24f, 0.33f, 0.48f, 1.0f};
        }

        out.push_back(DrawCommand{
            DrawCommandType::FillRect,
            node->layout,
            fill
        });
        out.push_back(DrawCommand{
            DrawCommandType::StrokeRect,
            node->layout,
            gfx::Color{0.90f, 0.94f, 0.99f, 1.0f},
            2.0f
        });
        out.push_back(DrawCommand{
            DrawCommandType::Text,
            node->layout,
            gfx::Color{0.94f, 0.97f, 1.0f, 1.0f},
            1.0f,
            node->layout.x + 16.0f,
            node->layout.y + (node->layout.h * 0.62f),
            node->props.text
        });
    }

    if(node->kind == NodeKind::TextEdit) {
        gfx::Color fill{0.12f, 0.16f, 0.23f, 1.0f};
        if(node->state.focused) {
            fill = gfx::Color{0.15f, 0.20f, 0.30f, 1.0f};
        } else if(node->state.hovered) {
            fill = gfx::Color{0.14f, 0.18f, 0.26f, 1.0f};
        }

        const gfx::Color border = node->state.focused
            ? gfx::Color{0.67f, 0.80f, 0.99f, 1.0f}
            : gfx::Color{0.55f, 0.62f, 0.74f, 1.0f};

        out.push_back(DrawCommand{
            DrawCommandType::FillRect,
            node->layout,
            fill
        });
        out.push_back(DrawCommand{
            DrawCommandType::StrokeRect,
            node->layout,
            border,
            node->state.focused ? 2.5f : 2.0f
        });
        out.push_back(DrawCommand{
            .type = DrawCommandType::TextInput,
            .rect = node->layout,
            .color = gfx::Color{0.94f, 0.97f, 1.0f, 1.0f},
            .thickness = 1.0f,
            .text = node->props.text,
            .focused = node->state.focused,
            .cursor_index = node->state.text_cursor,
            .caret_visible = node->state.caret_visible,
            .selection_anchor_index = node->state.text_selection_anchor,
            .has_selection_background_color = node->props.has_selection_background_color,
            .selection_background_color = node->props.selection_background_color,
            .has_selection_text_color = node->props.has_selection_text_color,
            .selection_text_color = node->props.selection_text_color
        });
    }

    if(node->kind == NodeKind::Custom) {
        out.push_back(DrawCommand{
            DrawCommandType::StrokeRect,
            node->layout,
            gfx::Color{0.78f, 0.82f, 0.88f, 1.0f},
            1.0f
        });
    }

    for(const std::size_t child_index : node->children) {
        emit_draw_commands(tree, child_index, out);
    }

    if(clips_children) {
        out.push_back(DrawCommand{
            DrawCommandType::PopClip
        });
    }
}

[[nodiscard]] std::vector<std::size_t> collect_layout_roots(const UiTree& tree)
{
    std::vector<std::size_t> roots;
    roots.reserve(tree.nodes().size());

    for(std::size_t i = 0U; i < tree.nodes().size(); ++i) {
        const UiNode* node = tree.get(i);
        if(node == nullptr || !has_flag(node->dirty_flags, DirtyFlags::Layout)) {
            continue;
        }

        bool has_dirty_ancestor = false;
        std::size_t parent = node->parent;
        while(parent != UiNode::npos) {
            const UiNode* parent_node = tree.get(parent);
            if(parent_node == nullptr) {
                break;
            }
            if(has_flag(parent_node->dirty_flags, DirtyFlags::Layout)) {
                has_dirty_ancestor = true;
                break;
            }
            parent = parent_node->parent;
        }

        if(!has_dirty_ancestor) {
            roots.push_back(i);
        }
    }

    return roots;
}

void ensure_paint_regions(UiTree& tree)
{
    for(std::size_t i = 0U; i < tree.nodes().size(); ++i) {
        UiNode* node = tree.get(i);
        if(node == nullptr || !has_flag(node->dirty_flags, DirtyFlags::Paint)) {
            continue;
        }
        tree.invalidate(rect_union(node->previous_layout, node->layout));
    }
}

[[nodiscard]] gfx::Rect command_bounds(const DrawCommand& command) noexcept
{
    switch(command.type) {
    case DrawCommandType::FillRect:
    case DrawCommandType::StrokeRect:
    case DrawCommandType::Text:
    case DrawCommandType::TextInput:
    case DrawCommandType::Image:
    case DrawCommandType::PushClip:
        return command.rect;
    case DrawCommandType::PopClip:
        break;
    }
    return gfx::Rect{};
}

[[nodiscard]] bool intersects_dirty_region(const DrawCommand& command, gfx::Rect dirty_region) noexcept
{
    const gfx::Rect bounds = command_bounds(command);
    if(rect_is_empty(bounds) || rect_is_empty(dirty_region)) {
        return false;
    }
    return rect_intersects(bounds, dirty_region);
}

void execute_draw_command(
    gfx::Renderer& renderer,
    const DrawCommand& command,
    const asset::AssetManager* assets
)
{
    switch(command.type) {
    case DrawCommandType::FillRect:
        renderer.fill_rect(command.rect, command.color);
        break;
    case DrawCommandType::StrokeRect:
        renderer.stroke_rect(command.rect, command.color, command.thickness);
        break;
    case DrawCommandType::Text:
        renderer.draw_text(command.text_x, command.text_y, command.text, command.color);
        break;
    case DrawCommandType::Image: {
        std::string image_source = command.image_source;
        if(assets != nullptr) {
            image_source = assets->resolve_reference(command.image_source);
        }

        if(image_source.empty()) {
            break;
        }

        renderer.draw_image(command.rect, image_source, command.color);
        break;
    }
    case DrawCommandType::TextInput: {
        const float padding_x = 12.0f;
        const float padding_y = 8.0f;
        const gfx::Rect text_clip{
            command.rect.x + padding_x,
            command.rect.y + padding_y,
            std::max(0.0f, command.rect.w - (2.0f * padding_x)),
            std::max(0.0f, command.rect.h - (2.0f * padding_y))
        };

        renderer.push_clip(text_clip);

        const float baseline_y = command.rect.y + (command.rect.h * 0.62f);
        const std::size_t cursor_index = clamp_utf8_cursor(command.text, command.cursor_index);
        const std::size_t selection_anchor_index =
            clamp_utf8_cursor(command.text, command.selection_anchor_index);
        const std::size_t selection_begin = std::min(cursor_index, selection_anchor_index);
        const std::size_t selection_limit = std::max(cursor_index, selection_anchor_index);
        const std::string text_before_cursor = command.text.substr(0U, cursor_index);

        const float text_width = renderer.measure_text(command.text);
        const float cursor_raw_x = renderer.measure_text(text_before_cursor);
        const float max_scroll = std::max(0.0f, text_width - text_clip.w);
        const float desired_scroll = std::max(0.0f, cursor_raw_x - std::max(0.0f, text_clip.w - 4.0f));
        const float scroll_x = std::clamp(desired_scroll, 0.0f, max_scroll);

        const bool has_selection = command.focused && selection_begin != selection_limit;
        float clipped_selection_x = text_clip.x;
        float clipped_selection_w = 0.0f;
        const float text_x = text_clip.x - scroll_x;
        if(has_selection) {
            const float before_selection_w =
                renderer.measure_text(command.text.substr(0U, selection_begin));
            const float selected_w = renderer.measure_text(
                command.text.substr(selection_begin, selection_limit - selection_begin)
            );
            const float selection_x = text_clip.x + before_selection_w - scroll_x;
            const float selection_right = selection_x + selected_w;
            const float clip_x0 = std::max(text_clip.x, selection_x);
            const float clip_x1 = std::min(text_clip.x + text_clip.w, selection_right);
            clipped_selection_x = clip_x0;
            clipped_selection_w = std::max(0.0f, clip_x1 - clip_x0);
            if(clipped_selection_w > 0.0f) {
                renderer.fill_rect(
                    gfx::Rect{
                        clipped_selection_x,
                        command.rect.y + 10.0f,
                        clipped_selection_w,
                        std::max(0.0f, command.rect.h - 20.0f)
                    },
                    command.has_selection_background_color
                        ? command.selection_background_color
                        : gfx::Color{0.02f, 0.02f, 0.02f, 0.88f}
                );
            }
        }
        renderer.draw_text(text_x, baseline_y, command.text, command.color);
        if(has_selection && command.has_selection_text_color && clipped_selection_w > 0.0f) {
            renderer.push_clip(gfx::Rect{
                clipped_selection_x,
                text_clip.y,
                clipped_selection_w,
                text_clip.h
            });
            renderer.draw_text(text_x, baseline_y, command.text, command.selection_text_color);
            renderer.pop_clip();
        }

        if(command.focused && command.caret_visible) {
            float caret_x = text_clip.x + cursor_raw_x - scroll_x;
            const float min_caret_x = text_clip.x;
            const float max_caret_x = text_clip.x + std::max(0.0f, text_clip.w - 2.0f);
            caret_x = std::clamp(caret_x, min_caret_x, max_caret_x);
            renderer.fill_rect(
                gfx::Rect{
                    caret_x,
                    command.rect.y + 10.0f,
                    2.0f,
                    std::max(0.0f, command.rect.h - 20.0f)
                },
                command.color
            );
        }

        renderer.pop_clip();
        break;
    }
    case DrawCommandType::PushClip:
        renderer.push_clip(command.rect);
        break;
    case DrawCommandType::PopClip:
        renderer.pop_clip();
        break;
    }
}

} // namespace

void Pipeline::layout(UiTree& tree, gfx::Rect viewport) const
{
    layout(tree, viewport, static_cast<gfx::Renderer*>(nullptr));
}

void Pipeline::layout(UiTree& tree, gfx::Rect viewport, gfx::Renderer* renderer) const
{
    const std::size_t root_index = tree.root();
    UiNode* root = tree.get(root_index);
    if(root == nullptr) {
        return;
    }

    const gfx::Rect old_root_layout = root->layout;
    if(!rect_equal(old_root_layout, viewport)) {
        root->previous_layout = old_root_layout;
        root->layout = viewport;
        tree.mark_subtree_dirty(root_index, DirtyFlags::Layout | DirtyFlags::Paint | DirtyFlags::Structure);
        tree.invalidate(rect_union(old_root_layout, viewport));
    }

    root->has_explicit_layout = true;

    if(!tree.has_any_dirty(DirtyFlags::Layout)) {
        return;
    }

    const std::vector<std::size_t> layout_roots = collect_layout_roots(tree);
    for(const std::size_t layout_root : layout_roots) {
        layout_subtree(tree, layout_root, renderer);
    }

    tree.clear_all_dirty(DirtyFlags::Layout | DirtyFlags::Structure);
}

std::vector<DrawCommand> Pipeline::build_draw_commands(const UiTree& tree) const
{
    std::vector<DrawCommand> commands;
    if(!tree.is_valid(tree.root())) {
        return commands;
    }

    commands.reserve(tree.nodes().size() * 3U);
    emit_draw_commands(tree, tree.root(), commands);
    return commands;
}

void Pipeline::paint(gfx::Renderer& renderer, const std::vector<DrawCommand>& commands) const
{
    paint(renderer, commands, static_cast<const asset::AssetManager*>(nullptr));
}

void Pipeline::paint(
    gfx::Renderer& renderer,
    const std::vector<DrawCommand>& commands,
    const asset::AssetManager& assets
) const
{
    paint(renderer, commands, &assets);
}

void Pipeline::paint(
    gfx::Renderer& renderer,
    const std::vector<DrawCommand>& commands,
    const asset::AssetManager* assets
) const
{
    for(const DrawCommand& command : commands) {
        execute_draw_command(renderer, command, assets);
    }
}

void Pipeline::paint(
    gfx::Renderer& renderer,
    UiTree& tree,
    const std::vector<DrawCommand>& commands,
    std::optional<gfx::Color> clear_color
) const
{
    paint(
        renderer,
        tree,
        commands,
        static_cast<const asset::AssetManager*>(nullptr),
        clear_color
    );
}

void Pipeline::paint(
    gfx::Renderer& renderer,
    UiTree& tree,
    const std::vector<DrawCommand>& commands,
    const asset::AssetManager& assets,
    std::optional<gfx::Color> clear_color
) const
{
    paint(renderer, tree, commands, &assets, clear_color);
}

void Pipeline::paint(
    gfx::Renderer& renderer,
    UiTree& tree,
    const std::vector<DrawCommand>& commands,
    const asset::AssetManager* assets,
    std::optional<gfx::Color> clear_color
) const
{
    ensure_paint_regions(tree);
    if(clear_color.has_value()) {
        // Double-buffered back buffers are not guaranteed to preserve prior
        // contents after swap; redraw the full frame when a clear color is used.
        renderer.clear(*clear_color);
        for(const DrawCommand& command : commands) {
            execute_draw_command(renderer, command, assets);
        }
        tree.clear_all_dirty(DirtyFlags::All);
        tree.clear_dirty_regions();
        return;
    }

    const std::vector<gfx::Rect>& dirty_regions = tree.dirty_regions();
    if(dirty_regions.empty()) {
        tree.clear_all_dirty(DirtyFlags::All);
        return;
    }

    for(const gfx::Rect dirty : dirty_regions) {
        if(rect_is_empty(dirty)) {
            continue;
        }

        renderer.push_clip(dirty);
        for(const DrawCommand& command : commands) {
            if(command.type == DrawCommandType::PushClip
                || command.type == DrawCommandType::PopClip
                || intersects_dirty_region(command, dirty)) {
                execute_draw_command(renderer, command, assets);
            }
        }

        renderer.pop_clip();
    }

    tree.clear_all_dirty(DirtyFlags::All);
    tree.clear_dirty_regions();
}

} // namespace guinevere::ui
