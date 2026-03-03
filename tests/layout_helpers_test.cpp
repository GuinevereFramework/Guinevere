#include <cmath>
#include <stdexcept>

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
    using AxisTrackConstraint = guinevere::ui::LayoutConfig::AxisTrackConstraint;

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
    auto responsive_box_entry = frame_builder.view("root", "responsive_box");
    responsive_box_entry.width(ResponsiveProperty{
        .compact = 100.0f,
        .expanded = 300.0f
    });
    responsive_box_entry.height(ResponsiveProperty{
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

    {
        const std::vector<float> tracks = guinevere::ui::resolve_axis_tracks(
            800.0f,
            std::vector<AxisTrackConstraint>{
                AxisTrackConstraint{
                    .min_size = 100.0f,
                    .preferred_size = 300.0f,
                    .max_size = 0.0f,
                    .grow_weight = 0.0f,
                    .shrink_priority = 1
                },
                AxisTrackConstraint{
                    .min_size = 200.0f,
                    .preferred_size = 200.0f,
                    .max_size = 0.0f,
                    .grow_weight = 1.0f,
                    .shrink_priority = 2
                },
                AxisTrackConstraint{
                    .min_size = 50.0f,
                    .preferred_size = 50.0f,
                    .max_size = 50.0f,
                    .grow_weight = 0.0f,
                    .shrink_priority = 3
                }
            },
            20.0f,
            10.0f
        );
        if(tracks.size() != 3U) {
            return 1;
        }
        if(!approx(tracks[0], 300.0f) || !approx(tracks[1], 390.0f) || !approx(tracks[2], 50.0f)) {
            return 1;
        }
    }

    {
        const std::vector<float> tracks = guinevere::ui::resolve_axis_tracks(
            500.0f,
            std::vector<AxisTrackConstraint>{
                AxisTrackConstraint{
                    .min_size = 100.0f,
                    .preferred_size = 300.0f,
                    .max_size = 0.0f,
                    .grow_weight = 0.0f,
                    .shrink_priority = 0
                },
                AxisTrackConstraint{
                    .min_size = 100.0f,
                    .preferred_size = 300.0f,
                    .max_size = 0.0f,
                    .grow_weight = 0.0f,
                    .shrink_priority = 10
                }
            },
            0.0f,
            10.0f
        );
        if(tracks.size() != 2U) {
            return 1;
        }
        if(!approx(tracks[0], 190.0f) || !approx(tracks[1], 300.0f)) {
            return 1;
        }
    }

    {
        const std::vector<float> tracks = guinevere::ui::resolve_axis_tracks(
            100.0f,
            std::vector<AxisTrackConstraint>{
                AxisTrackConstraint{
                    .min_size = 80.0f,
                    .preferred_size = 80.0f,
                    .max_size = 0.0f,
                    .grow_weight = 0.0f,
                    .shrink_priority = 1
                },
                AxisTrackConstraint{
                    .min_size = 80.0f,
                    .preferred_size = 80.0f,
                    .max_size = 0.0f,
                    .grow_weight = 0.0f,
                    .shrink_priority = 1
                }
            },
            0.0f,
            10.0f
        );
        if(tracks.size() != 2U) {
            return 1;
        }
        if(!approx(tracks[0] + tracks[1], 90.0f)) {
            return 1;
        }
    }

    {
        guinevere::ui::UiTree tree;
        guinevere::ui::Pipeline pipeline;
        std::vector<ReconciledNode> tracks_frame;
        FrameBuilder tracks_frame_builder(tracks_frame);

        auto root_panel = tracks_frame_builder.panel("root", "layout_root");
        root_panel.layout(Rect{0.0f, 0.0f, 400.0f, 400.0f});
        root_panel.column(10.0f, 20.0f);
        root_panel.main_axis_tracks(std::vector<AxisTrackConstraint>{
            AxisTrackConstraint{
                .min_size = 100.0f,
                .preferred_size = 100.0f,
                .max_size = 100.0f,
                .grow_weight = 0.0f,
                .shrink_priority = 1
            },
            AxisTrackConstraint{
                .min_size = 80.0f,
                .preferred_size = 100.0f,
                .max_size = 0.0f,
                .grow_weight = 1.0f,
                .shrink_priority = 2
            },
            AxisTrackConstraint{
                .min_size = 40.0f,
                .preferred_size = 40.0f,
                .max_size = 40.0f,
                .grow_weight = 0.0f,
                .shrink_priority = 3
            }
        });

        (void)tracks_frame_builder.panel("layout_root", "track_a");
        (void)tracks_frame_builder.panel("layout_root", "track_b");
        (void)tracks_frame_builder.panel("layout_root", "track_c");

        guinevere::ui::Reconciler::reconcile(tree, "root", tracks_frame);
        pipeline.layout(tree, Rect{0.0f, 0.0f, 400.0f, 400.0f});

        const std::size_t track_a_index = tree.find("track_a");
        const std::size_t track_b_index = tree.find("track_b");
        const std::size_t track_c_index = tree.find("track_c");
        if(track_a_index == guinevere::ui::UiTree::npos
            || track_b_index == guinevere::ui::UiTree::npos
            || track_c_index == guinevere::ui::UiTree::npos) {
            return 1;
        }

        const guinevere::ui::UiNode* track_a = tree.get(track_a_index);
        const guinevere::ui::UiNode* track_b = tree.get(track_b_index);
        const guinevere::ui::UiNode* track_c = tree.get(track_c_index);
        if(track_a == nullptr || track_b == nullptr || track_c == nullptr) {
            return 1;
        }

        if(!approx(track_a->layout.h, 100.0f)
            || !approx(track_b->layout.h, 200.0f)
            || !approx(track_c->layout.h, 40.0f)) {
            return 1;
        }
        if(!approx(track_a->layout.y, 20.0f)
            || !approx(track_b->layout.y, 130.0f)
            || !approx(track_c->layout.y, 340.0f)) {
            return 1;
        }
    }

    {
        bool duplicate_key_rejected = false;
        std::vector<ReconciledNode> duplicate_frame;
        FrameBuilder duplicate_builder(duplicate_frame);
        try {
            (void)duplicate_builder.panel("root", "duplicate");
            (void)duplicate_builder.button("root", "duplicate", "Again");
        } catch(const std::invalid_argument&) {
            duplicate_key_rejected = true;
        }
        if(!duplicate_key_rejected) {
            return 1;
        }
    }

    {
        bool invalid_component_key_rejected = false;
        guinevere::ui::StateStore scoped_state_store;
        std::vector<ReconciledNode> component_frame;
        FrameBuilder component_builder(component_frame);
        guinevere::ui::ComponentScope component_scope(
            component_builder,
            scoped_state_store,
            "root",
            "demo_component"
        );
        try {
            (void)component_scope.panel("bad.key");
        } catch(const std::invalid_argument&) {
            invalid_component_key_rejected = true;
        }
        if(!invalid_component_key_rejected) {
            return 1;
        }
    }

    {
        bool invalid_state_key_rejected = false;
        guinevere::ui::StateStore scoped_state_store;
        auto scoped_state = scoped_state_store.scope("demo_state");
        try {
            (void)scoped_state.use<int>("bad.key", 0);
        } catch(const std::invalid_argument&) {
            invalid_state_key_rejected = true;
        }
        if(!invalid_state_key_rejected) {
            return 1;
        }
    }

    {
        bool unknown_parent_rejected = false;
        guinevere::ui::UiTree tree;
        std::vector<ReconciledNode> invalid_nodes;
        ReconciledNode orphan;
        orphan.parent_key = "missing.parent";
        orphan.node.key = "orphan";
        orphan.node.kind = guinevere::ui::NodeKind::Panel;
        invalid_nodes.push_back(orphan);
        try {
            guinevere::ui::Reconciler::reconcile(tree, "root", invalid_nodes);
        } catch(const std::invalid_argument&) {
            unknown_parent_rejected = true;
        }
        if(!unknown_parent_rejected) {
            return 1;
        }
    }

    return 0;
}
