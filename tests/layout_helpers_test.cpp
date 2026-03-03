#include <cmath>

#include <guinevere/ui/runtime.hpp>

namespace {

[[nodiscard]] bool approx(float lhs, float rhs) noexcept
{
    return std::fabs(lhs - rhs) <= 0.001f;
}

[[nodiscard]] bool rect_eq(
    guinevere::gfx::Rect rect,
    float x,
    float y,
    float w,
    float h
) noexcept
{
    return approx(rect.x, x)
        && approx(rect.y, y)
        && approx(rect.w, w)
        && approx(rect.h, h);
}

} // namespace

int main()
{
    using guinevere::gfx::Rect;
    using guinevere::ui::EdgeInsets;
    using guinevere::ui::FrameBuilder;
    using guinevere::ui::ReconciledNode;
    using guinevere::ui::RectSplit;
    using guinevere::ui::ResponsiveProperty;

    const Rect inset_uniform = guinevere::ui::inset_rect(Rect{10.0f, 20.0f, 100.0f, 60.0f}, 8.0f);
    if(!rect_eq(inset_uniform, 18.0f, 28.0f, 84.0f, 44.0f)) {
        return 1;
    }

    const Rect inset_edges = guinevere::ui::inset_rect(
        Rect{10.0f, 20.0f, 100.0f, 60.0f},
        EdgeInsets{5.0f, 6.0f, 7.0f, 8.0f}
    );
    if(!rect_eq(inset_edges, 15.0f, 26.0f, 88.0f, 46.0f)) {
        return 1;
    }

    const RectSplit row_start = guinevere::ui::split_row_start(
        Rect{0.0f, 0.0f, 300.0f, 40.0f},
        120.0f,
        16.0f
    );
    if(!rect_eq(row_start.start, 0.0f, 0.0f, 120.0f, 40.0f)) {
        return 1;
    }
    if(!rect_eq(row_start.end, 136.0f, 0.0f, 164.0f, 40.0f)) {
        return 1;
    }

    const RectSplit row_end = guinevere::ui::split_row_end(
        Rect{0.0f, 0.0f, 300.0f, 40.0f},
        120.0f,
        16.0f
    );
    if(!rect_eq(row_end.start, 0.0f, 0.0f, 164.0f, 40.0f)) {
        return 1;
    }
    if(!rect_eq(row_end.end, 180.0f, 0.0f, 120.0f, 40.0f)) {
        return 1;
    }

    const RectSplit row_ratio_end = guinevere::ui::split_row_ratio_end(
        Rect{0.0f, 0.0f, 500.0f, 80.0f},
        0.40f,
        20.0f,
        220.0f,
        260.0f
    );
    if(!rect_eq(row_ratio_end.start, 0.0f, 0.0f, 260.0f, 80.0f)) {
        return 1;
    }
    if(!rect_eq(row_ratio_end.end, 280.0f, 0.0f, 220.0f, 80.0f)) {
        return 1;
    }

    const RectSplit column_start = guinevere::ui::split_column_start(
        Rect{4.0f, 8.0f, 160.0f, 300.0f},
        110.0f,
        12.0f
    );
    if(!rect_eq(column_start.start, 4.0f, 8.0f, 160.0f, 110.0f)) {
        return 1;
    }
    if(!rect_eq(column_start.end, 4.0f, 130.0f, 160.0f, 178.0f)) {
        return 1;
    }

    const RectSplit row_gap_clamped = guinevere::ui::split_row_start(
        Rect{0.0f, 0.0f, 100.0f, 20.0f},
        95.0f,
        20.0f
    );
    if(!rect_eq(row_gap_clamped.start, 0.0f, 0.0f, 80.0f, 20.0f)) {
        return 1;
    }
    if(!rect_eq(row_gap_clamped.end, 100.0f, 0.0f, 0.0f, 20.0f)) {
        return 1;
    }

    const float medium_width = guinevere::ui::resolve_responsive_property(
        guinevere::ui::AppBreakpoint::Medium,
        ResponsiveProperty{
            .compact = 100.0f,
            .expanded = 300.0f
        }
    );
    if(!approx(medium_width, 200.0f)) {
        return 1;
    }

    const float fallback_width = guinevere::ui::resolve_responsive_property(
        guinevere::ui::AppBreakpoint::Compact,
        ResponsiveProperty{},
        42.0f
    );
    if(!approx(fallback_width, 42.0f)) {
        return 1;
    }

    if(std::string_view(guinevere::ui::app_breakpoint_label(guinevere::ui::AppBreakpoint::Compact))
        != "compact") {
        return 1;
    }
    if(std::string_view(guinevere::ui::app_breakpoint_label(guinevere::ui::AppBreakpoint::Medium))
        != "medium") {
        return 1;
    }
    if(std::string_view(guinevere::ui::app_breakpoint_label(guinevere::ui::AppBreakpoint::Expanded))
        != "expanded") {
        return 1;
    }

    std::vector<ReconciledNode> frame;
    FrameBuilder frame_builder(frame);
    frame_builder.app_breakpoint(guinevere::ui::AppBreakpoint::Expanded);
    frame_builder
        .view("root", "responsive_box")
        .width(ResponsiveProperty{
            .compact = 100.0f,
            .expanded = 300.0f
        })
        .height(ResponsiveProperty{
            .compact = 40.0f,
            .expanded = 80.0f
        });
    if(frame.empty()) {
        return 1;
    }

    const auto& layout_config = frame.front().node.layout_config;
    if(layout_config.width_mode != guinevere::ui::SizeMode::Fixed
        || !approx(layout_config.fixed_width, 300.0f)) {
        return 1;
    }
    if(layout_config.height_mode != guinevere::ui::SizeMode::Fixed
        || !approx(layout_config.fixed_height, 80.0f)) {
        return 1;
    }

    return 0;
}
