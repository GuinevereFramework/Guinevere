#include <algorithm>

#include <guinevere/ui/runtime.hpp>

namespace guinevere::ui {

const char* app_breakpoint_label(AppBreakpoint breakpoint) noexcept
{
    switch(breakpoint) {
    case AppBreakpoint::Compact:
        return "compact";
    case AppBreakpoint::Medium:
        return "medium";
    case AppBreakpoint::Expanded:
        return "expanded";
    }

    return "compact";
}

AppBreakpoint detect_app_breakpoint(
    float viewport_width,
    float medium_breakpoint_min_width,
    float expanded_breakpoint_min_width
) noexcept
{
    const float clamped_width = std::max(0.0f, viewport_width);
    if(clamped_width >= expanded_breakpoint_min_width) {
        return AppBreakpoint::Expanded;
    }
    if(clamped_width >= medium_breakpoint_min_width) {
        return AppBreakpoint::Medium;
    }
    return AppBreakpoint::Compact;
}

float resolve_responsive_scalar(AppBreakpoint breakpoint, const ResponsiveScalar& value) noexcept
{
    switch(breakpoint) {
    case AppBreakpoint::Compact:
        return value.compact;
    case AppBreakpoint::Medium:
        return value.medium;
    case AppBreakpoint::Expanded:
        return value.expanded;
    }

    return value.compact;
}

float resolve_responsive_property(
    AppBreakpoint breakpoint,
    const ResponsiveProperty& value,
    float fallback
) noexcept
{
    const auto normalize = [](const std::optional<float>& candidate) -> std::optional<float> {
        if(!candidate.has_value()) {
            return std::nullopt;
        }
        return std::max(0.0f, *candidate);
    };

    const std::optional<float> compact = normalize(value.compact);
    const std::optional<float> medium = normalize(value.medium);
    const std::optional<float> expanded = normalize(value.expanded);

    switch(breakpoint) {
    case AppBreakpoint::Compact:
        if(compact.has_value()) {
            return *compact;
        }
        if(medium.has_value()) {
            return *medium;
        }
        if(expanded.has_value()) {
            return *expanded;
        }
        break;
    case AppBreakpoint::Medium:
        if(medium.has_value()) {
            return *medium;
        }
        if(compact.has_value() && expanded.has_value()) {
            return resolve_axis_size((*compact + *expanded) * 0.5f);
        }
        if(compact.has_value()) {
            return *compact;
        }
        if(expanded.has_value()) {
            return *expanded;
        }
        break;
    case AppBreakpoint::Expanded:
        if(expanded.has_value()) {
            return *expanded;
        }
        if(medium.has_value()) {
            return *medium;
        }
        if(compact.has_value()) {
            return *compact;
        }
        break;
    }

    return std::max(0.0f, fallback);
}

AppLayoutResult resolve_app_layout(gfx::Rect viewport, const AppLayoutSpec& spec) noexcept
{
    AppLayoutResult out{};
    out.viewport = viewport;
    out.breakpoint = detect_app_breakpoint(
        viewport.w,
        spec.medium_breakpoint_min_width,
        spec.expanded_breakpoint_min_width
    );

    const ViewportLayoutConstraints constraints{
        .margin_left = resolve_responsive_scalar(out.breakpoint, spec.margin_left)
            + std::max(0.0f, spec.safe_area.left),
        .margin_top = resolve_responsive_scalar(out.breakpoint, spec.margin_top)
            + std::max(0.0f, spec.safe_area.top),
        .margin_right = resolve_responsive_scalar(out.breakpoint, spec.margin_right)
            + std::max(0.0f, spec.safe_area.right),
        .margin_bottom = resolve_responsive_scalar(out.breakpoint, spec.margin_bottom)
            + std::max(0.0f, spec.safe_area.bottom),
        .min_width = spec.min_width,
        .max_width = spec.max_width,
        .min_height = spec.min_height,
        .max_height = spec.max_height,
        .fill_viewport_width_below = spec.fill_viewport_width_below,
        .fill_viewport_height_below = spec.fill_viewport_height_below,
        .clamp_to_viewport = spec.clamp_to_viewport
    };
    out.container = layout_in_viewport(viewport, constraints);
    return out;
}

AppScaffoldResult resolve_app_scaffold(gfx::Rect viewport, const AppScaffoldSpec& spec) noexcept
{
    AppScaffoldResult out{};
    out.app_layout = resolve_app_layout(viewport, spec.app_layout);

    const gfx::Rect container = out.app_layout.container;
    const float raw_header_height =
        resolve_responsive_scalar(out.app_layout.breakpoint, spec.header_height);
    const float header_height = resolve_axis_size(raw_header_height, 0.0f, container.h);

    float header_gap = 0.0f;
    if(header_height > 0.0f) {
        header_gap = std::max(
            0.0f,
            resolve_responsive_scalar(out.app_layout.breakpoint, spec.header_gap)
        );
        const float max_gap = std::max(0.0f, container.h - header_height);
        if(header_gap > max_gap) {
            header_gap = max_gap;
        }
    }

    out.header = gfx::Rect{
        container.x,
        container.y,
        container.w,
        header_height
    };
    out.body = gfx::Rect{
        container.x,
        container.y + header_height + header_gap,
        container.w,
        std::max(0.0f, container.h - header_height - header_gap)
    };

    return out;
}

gfx::Rect layout_in_viewport(gfx::Rect viewport, const ViewportLayoutConstraints& constraints) noexcept
{
    const float viewport_w = std::max(0.0f, viewport.w);
    const float viewport_h = std::max(0.0f, viewport.h);

    const float margin_left = std::max(0.0f, constraints.margin_left);
    const float margin_top = std::max(0.0f, constraints.margin_top);
    const float margin_right = std::max(0.0f, constraints.margin_right);
    const float margin_bottom = std::max(0.0f, constraints.margin_bottom);

    float width = viewport_w - (margin_left + margin_right);
    float height = viewport_h - (margin_top + margin_bottom);
    if(width < 0.0f) {
        width = 0.0f;
    }
    if(height < 0.0f) {
        height = 0.0f;
    }

    width = resolve_axis_size(width, constraints.min_width, constraints.max_width);
    height = resolve_axis_size(height, constraints.min_height, constraints.max_height);

    if(constraints.clamp_to_viewport) {
        width = std::min(width, viewport_w);
        height = std::min(height, viewport_h);
    }

    if(constraints.fill_viewport_width_below > 0.0f
        && width < constraints.fill_viewport_width_below) {
        width = viewport_w;
    }
    if(constraints.fill_viewport_height_below > 0.0f
        && height < constraints.fill_viewport_height_below) {
        height = viewport_h;
    }

    return gfx::Rect{
        viewport.x + margin_left,
        viewport.y + margin_top,
        width,
        height
    };
}

float axis_available_size(
    float container_size,
    float padding,
    float reserved_size,
    float gap,
    std::size_t gap_count
) noexcept
{
    const float clamped_container_size = std::max(0.0f, container_size);
    const float clamped_padding = std::max(0.0f, padding);
    const float clamped_reserved_size = std::max(0.0f, reserved_size);
    const float clamped_gap = std::max(0.0f, gap);
    const float total_gap = clamped_gap * static_cast<float>(gap_count);
    const float used_size = (clamped_padding * 2.0f) + clamped_reserved_size + total_gap;
    return std::max(0.0f, clamped_container_size - used_size);
}

float resolve_axis_size(float preferred_size, float min_size, float max_size) noexcept
{
    float out = std::max(0.0f, preferred_size);
    const float clamped_min_size = std::max(0.0f, min_size);
    if(max_size > 0.0f && out > max_size) {
        out = max_size;
    }
    if(out < clamped_min_size) {
        out = clamped_min_size;
    }
    return std::max(0.0f, out);
}

gfx::Rect inset_rect(gfx::Rect rect, float inset) noexcept
{
    const float clamped_inset = std::max(0.0f, inset);
    return inset_rect(
        rect,
        EdgeInsets{clamped_inset, clamped_inset, clamped_inset, clamped_inset}
    );
}

gfx::Rect inset_rect(gfx::Rect rect, const EdgeInsets& insets) noexcept
{
    const float left = std::max(0.0f, insets.left);
    const float top = std::max(0.0f, insets.top);
    const float right = std::max(0.0f, insets.right);
    const float bottom = std::max(0.0f, insets.bottom);

    const float base_width = std::max(0.0f, rect.w);
    const float base_height = std::max(0.0f, rect.h);

    return gfx::Rect{
        rect.x + left,
        rect.y + top,
        std::max(0.0f, base_width - (left + right)),
        std::max(0.0f, base_height - (top + bottom))
    };
}

namespace {

[[nodiscard]] float clamp_split_size(float axis_size, float size, float gap) noexcept
{
    const float clamped_axis = std::max(0.0f, axis_size);
    const float clamped_gap = std::max(0.0f, gap);
    const float available_axis = std::max(0.0f, clamped_axis - clamped_gap);
    return std::min(resolve_axis_size(size), available_axis);
}

[[nodiscard]] float resolve_ratio_split_size(
    float axis_size,
    float ratio,
    float min_size,
    float max_size
) noexcept
{
    const float clamped_axis = std::max(0.0f, axis_size);
    const float clamped_ratio = std::max(0.0f, ratio);
    const float preferred_size = clamped_axis * clamped_ratio;
    return resolve_axis_size(preferred_size, min_size, max_size);
}

} // namespace

RectSplit split_row_start(gfx::Rect rect, float start_width, float gap) noexcept
{
    RectSplit out{};
    const float clamped_height = std::max(0.0f, rect.h);
    const float start_size = clamp_split_size(rect.w, start_width, gap);
    const float remaining_before_gap = std::max(0.0f, std::max(0.0f, rect.w) - start_size);
    const float actual_gap = std::min(std::max(0.0f, gap), remaining_before_gap);
    const float end_size = std::max(0.0f, remaining_before_gap - actual_gap);

    out.start = gfx::Rect{rect.x, rect.y, start_size, clamped_height};
    out.end = gfx::Rect{
        rect.x + start_size + actual_gap,
        rect.y,
        end_size,
        clamped_height
    };
    return out;
}

RectSplit split_row_end(gfx::Rect rect, float end_width, float gap) noexcept
{
    RectSplit out{};
    const float clamped_height = std::max(0.0f, rect.h);
    const float end_size = clamp_split_size(rect.w, end_width, gap);
    const float remaining_before_gap = std::max(0.0f, std::max(0.0f, rect.w) - end_size);
    const float actual_gap = std::min(std::max(0.0f, gap), remaining_before_gap);
    const float start_size = std::max(0.0f, remaining_before_gap - actual_gap);

    out.start = gfx::Rect{rect.x, rect.y, start_size, clamped_height};
    out.end = gfx::Rect{
        rect.x + start_size + actual_gap,
        rect.y,
        end_size,
        clamped_height
    };
    return out;
}

RectSplit split_column_start(gfx::Rect rect, float start_height, float gap) noexcept
{
    RectSplit out{};
    const float clamped_width = std::max(0.0f, rect.w);
    const float start_size = clamp_split_size(rect.h, start_height, gap);
    const float remaining_before_gap = std::max(0.0f, std::max(0.0f, rect.h) - start_size);
    const float actual_gap = std::min(std::max(0.0f, gap), remaining_before_gap);
    const float end_size = std::max(0.0f, remaining_before_gap - actual_gap);

    out.start = gfx::Rect{rect.x, rect.y, clamped_width, start_size};
    out.end = gfx::Rect{
        rect.x,
        rect.y + start_size + actual_gap,
        clamped_width,
        end_size
    };
    return out;
}

RectSplit split_column_end(gfx::Rect rect, float end_height, float gap) noexcept
{
    RectSplit out{};
    const float clamped_width = std::max(0.0f, rect.w);
    const float end_size = clamp_split_size(rect.h, end_height, gap);
    const float remaining_before_gap = std::max(0.0f, std::max(0.0f, rect.h) - end_size);
    const float actual_gap = std::min(std::max(0.0f, gap), remaining_before_gap);
    const float start_size = std::max(0.0f, remaining_before_gap - actual_gap);

    out.start = gfx::Rect{rect.x, rect.y, clamped_width, start_size};
    out.end = gfx::Rect{
        rect.x,
        rect.y + start_size + actual_gap,
        clamped_width,
        end_size
    };
    return out;
}

RectSplit split_row_ratio_start(
    gfx::Rect rect,
    float start_ratio,
    float gap,
    float min_size,
    float max_size
) noexcept
{
    const float start_size = resolve_ratio_split_size(
        rect.w,
        start_ratio,
        min_size,
        max_size
    );
    return split_row_start(rect, start_size, gap);
}

RectSplit split_row_ratio_end(
    gfx::Rect rect,
    float end_ratio,
    float gap,
    float min_size,
    float max_size
) noexcept
{
    const float end_size = resolve_ratio_split_size(
        rect.w,
        end_ratio,
        min_size,
        max_size
    );
    return split_row_end(rect, end_size, gap);
}

RectSplit split_column_ratio_start(
    gfx::Rect rect,
    float start_ratio,
    float gap,
    float min_size,
    float max_size
) noexcept
{
    const float start_size = resolve_ratio_split_size(
        rect.h,
        start_ratio,
        min_size,
        max_size
    );
    return split_column_start(rect, start_size, gap);
}

RectSplit split_column_ratio_end(
    gfx::Rect rect,
    float end_ratio,
    float gap,
    float min_size,
    float max_size
) noexcept
{
    const float end_size = resolve_ratio_split_size(
        rect.h,
        end_ratio,
        min_size,
        max_size
    );
    return split_column_end(rect, end_size, gap);
}

} // namespace guinevere::ui
