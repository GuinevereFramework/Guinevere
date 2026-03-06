#pragma once

#include <cassert>
#include <unordered_set>

#include <guinevere/ui/types.hpp>

namespace guinevere::ui {

class FrameBuilder {
public:
    class Entry {
    public:
        explicit Entry(
            std::vector<ReconciledNode>* nodes,
            std::size_t node_index,
            const AppBreakpoint* app_breakpoint
        ) noexcept
            : nodes_(nodes)
            , node_index_(node_index)
            , app_breakpoint_(app_breakpoint)
        {
        }

        void text(std::string value)
        {
            node().node.text = std::move(value);
        }

        void image_source(std::string value)
        {
            node().node.image_source = std::move(value);
        }

        void image_asset(std::string asset_id)
        {
            constexpr std::string_view prefix = "asset://image/";
            node().node.image_source.clear();
            node().node.image_source.reserve(prefix.size() + asset_id.size());
            node().node.image_source.append(prefix);
            node().node.image_source.append(asset_id);
        }

        void image_tint(gfx::Color value)
        {
            node().node.image_tint = value;
        }

        void classes(std::vector<std::string> values)
        {
            node().node.classes = std::move(values);
        }

        template<typename OnClick>
        void on_click(OnClick&& on_click)
        {
            node().node.on_click = std::forward<OnClick>(on_click);
        }

        template<typename OnChange>
        void on_text_change(OnChange&& on_change)
        {
            node().node.on_text_change = std::forward<OnChange>(on_change);
        }

        template<typename OnSubmit>
        void on_text_submit(OnSubmit&& on_submit)
        {
            node().node.on_text_submit = std::forward<OnSubmit>(on_submit);
        }

        void allow_caret_toggle(bool value = true)
        {
            node().node.allow_user_toggle_caret = value;
        }

        void allow_arrow_left(bool value = true)
        {
            node().node.allow_arrow_left = value;
        }

        void allow_arrow_right(bool value = true)
        {
            node().node.allow_arrow_right = value;
        }

        void allow_arrow_up(bool value = true)
        {
            node().node.allow_arrow_up = value;
        }

        void allow_arrow_down(bool value = true)
        {
            node().node.allow_arrow_down = value;
        }

        void allow_ctrl_a(bool value = true)
        {
            node().node.allow_ctrl_a = value;
        }

        void allow_ctrl_c(bool value = true)
        {
            node().node.allow_ctrl_c = value;
        }

        void allow_ctrl_v(bool value = true)
        {
            node().node.allow_ctrl_v = value;
        }

        void allow_ctrl_x(bool value = true)
        {
            node().node.allow_ctrl_x = value;
        }

        void allow_clipboard_shortcuts(bool value = true)
        {
            node().node.allow_ctrl_a = value;
            node().node.allow_ctrl_c = value;
            node().node.allow_ctrl_v = value;
            node().node.allow_ctrl_x = value;
        }

        void allow_ctrl_shortcuts(bool value = true)
        {
            allow_clipboard_shortcuts(value);
        }

        void allow_arrow_keys(bool value = true)
        {
            node().node.allow_arrow_left = value;
            node().node.allow_arrow_right = value;
            node().node.allow_arrow_up = value;
            node().node.allow_arrow_down = value;
        }

        void allow_mouse_set_cursor(bool value = true)
        {
            node().node.allow_mouse_set_cursor = value;
        }

        void allow_mouse_drag_selection(bool value = true)
        {
            node().node.allow_mouse_drag_selection = value;
        }

        void selection_background_color(gfx::Color value)
        {
            node().node.has_selection_background_color = true;
            node().node.selection_background_color = value;
        }

        void selection_text_color(gfx::Color value)
        {
            node().node.has_selection_text_color = true;
            node().node.selection_text_color = value;
        }

        void selection_colors(gfx::Color background, gfx::Color text)
        {
            selection_background_color(background);
            selection_text_color(text);
        }

        void layout(gfx::Rect rect)
        {
            node().node.layout_hint = rect;
        }

        void column(float gap = 6.0f, float padding = 8.0f)
        {
            node().node.layout_config.direction = LayoutDirection::Column;
            node().node.layout_config.gap = gap;
            node().node.layout_config.padding = padding;
            node().node.layout_config.grid_columns.clear();
            node().node.layout_config.grid_rows.clear();
            node().node.layout_config.grid_auto_columns = 1U;
        }

        void column(
            std::initializer_list<AxisTrack> tracks,
            float gap = 6.0f,
            float padding = 8.0f
        )
        {
            column(gap, padding);
            main_axis_tracks(std::move(tracks));
        }

        void row(float gap = 6.0f, float padding = 8.0f)
        {
            node().node.layout_config.direction = LayoutDirection::Row;
            node().node.layout_config.gap = gap;
            node().node.layout_config.padding = padding;
            node().node.layout_config.grid_columns.clear();
            node().node.layout_config.grid_rows.clear();
            node().node.layout_config.grid_auto_columns = 1U;
        }

        void row(
            std::initializer_list<AxisTrack> tracks,
            float gap = 6.0f,
            float padding = 8.0f
        )
        {
            row(gap, padding);
            main_axis_tracks(std::move(tracks));
        }

        void align(AlignItems value)
        {
            node().node.layout_config.align_items = value;
        }

        void justify(JustifyContent value)
        {
            node().node.layout_config.justify_content = value;
        }

        void overflow(OverflowPolicy value)
        {
            node().node.layout_config.overflow = value;
        }

        void main_axis_tracks(std::vector<LayoutConfig::AxisTrackConstraint> tracks)
        {
            node().node.layout_config.main_axis_tracks = std::move(tracks);
        }

        void main_axis_tracks(std::initializer_list<AxisTrack> tracks)
        {
            node().node.layout_config.main_axis_tracks = axis_tracks(tracks);
        }

        void clear_main_axis_tracks()
        {
            node().node.layout_config.main_axis_tracks.clear();
        }

        void grid(std::size_t auto_columns = 1U, float gap = 6.0f, float padding = 8.0f)
        {
            node().node.layout_config.direction = LayoutDirection::Grid;
            node().node.layout_config.gap = gap;
            node().node.layout_config.padding = padding;
            node().node.layout_config.main_axis_tracks.clear();
            node().node.layout_config.grid_columns.clear();
            node().node.layout_config.grid_rows.clear();
            node().node.layout_config.grid_auto_columns = auto_columns > 0U ? auto_columns : 1U;
        }

        void grid(
            std::initializer_list<AxisTrack> columns,
            float gap = 6.0f,
            float padding = 8.0f
        )
        {
            grid(columns.size(), gap, padding);
            grid_columns(std::move(columns));
        }

        void grid_columns(std::vector<LayoutConfig::AxisTrackConstraint> tracks)
        {
            node().node.layout_config.grid_columns = std::move(tracks);
            if(!node().node.layout_config.grid_columns.empty()) {
                node().node.layout_config.grid_auto_columns = node().node.layout_config.grid_columns.size();
            }
        }

        void grid_columns(std::initializer_list<AxisTrack> tracks)
        {
            node().node.layout_config.grid_columns = axis_tracks(tracks);
            if(!node().node.layout_config.grid_columns.empty()) {
                node().node.layout_config.grid_auto_columns = node().node.layout_config.grid_columns.size();
            }
        }

        void grid_rows(std::vector<LayoutConfig::AxisTrackConstraint> tracks)
        {
            node().node.layout_config.grid_rows = std::move(tracks);
        }

        void grid_rows(std::initializer_list<AxisTrack> tracks)
        {
            node().node.layout_config.grid_rows = axis_tracks(tracks);
        }

        void clear_grid_tracks()
        {
            node().node.layout_config.grid_columns.clear();
            node().node.layout_config.grid_rows.clear();
            node().node.layout_config.grid_auto_columns = 1U;
        }

        void overflow_clip()
        {
            overflow(OverflowPolicy::Clip);
        }

        void overflow_visible()
        {
            overflow(OverflowPolicy::Visible);
        }

        void overflow_wrap()
        {
            overflow(OverflowPolicy::Wrap);
        }

        void overflow_scroll()
        {
            overflow(OverflowPolicy::Scroll);
        }

        void align_start()
        {
            align(AlignItems::Start);
        }

        void align_center()
        {
            align(AlignItems::Center);
        }

        void align_end()
        {
            align(AlignItems::End);
        }

        void align_stretch()
        {
            align(AlignItems::Stretch);
        }

        void justify_start()
        {
            justify(JustifyContent::Start);
        }

        void justify_center()
        {
            justify(JustifyContent::Center);
        }

        void justify_end()
        {
            justify(JustifyContent::End);
        }

        void justify_space_between()
        {
            justify(JustifyContent::SpaceBetween);
        }

        void width_fill(float weight = 1.0f)
        {
            node().node.layout_config.width_mode = SizeMode::Fill;
            node().node.layout_config.width_fill_weight = weight;
        }

        void height_fill(float weight = 1.0f)
        {
            node().node.layout_config.height_mode = SizeMode::Fill;
            node().node.layout_config.height_fill_weight = weight;
        }

        void width_fixed(float width)
        {
            node().node.layout_config.width_mode = SizeMode::Fixed;
            node().node.layout_config.fixed_width = width;
        }

        void width(float width)
        {
            width_fixed(width);
        }

        void width(const ResponsiveProperty& value)
        {
            const AppBreakpoint breakpoint = app_breakpoint_ != nullptr
                ? *app_breakpoint_
                : AppBreakpoint::Compact;
            width_fixed(resolve_responsive_property(breakpoint, value));
        }

        void height_fixed(float height)
        {
            node().node.layout_config.height_mode = SizeMode::Fixed;
            node().node.layout_config.fixed_height = height;
        }

        void height(float height)
        {
            height_fixed(height);
        }

        void height(const ResponsiveProperty& value)
        {
            const AppBreakpoint breakpoint = app_breakpoint_ != nullptr
                ? *app_breakpoint_
                : AppBreakpoint::Compact;
            height_fixed(resolve_responsive_property(breakpoint, value));
        }

        void size_fixed(float width, float height)
        {
            node().node.layout_config.width_mode = SizeMode::Fixed;
            node().node.layout_config.height_mode = SizeMode::Fixed;
            node().node.layout_config.fixed_width = width;
            node().node.layout_config.fixed_height = height;
        }

        void size_auto()
        {
            node().node.layout_config.width_mode = SizeMode::Auto;
            node().node.layout_config.height_mode = SizeMode::Auto;
        }

        void width_auto()
        {
            node().node.layout_config.width_mode = SizeMode::Auto;
        }

        void height_auto()
        {
            node().node.layout_config.height_mode = SizeMode::Auto;
        }

        void width_intrinsic()
        {
            width_auto();
        }

        void height_intrinsic()
        {
            height_auto();
        }

        void size_intrinsic()
        {
            size_auto();
        }

        void min_width(float value)
        {
            node().node.layout_config.min_width = value;
        }

        void max_width(float value)
        {
            node().node.layout_config.max_width = value;
        }

        void min_height(float value)
        {
            node().node.layout_config.min_height = value;
        }

        void max_height(float value)
        {
            node().node.layout_config.max_height = value;
        }

        void width_range(float min_value, float max_value)
        {
            node().node.layout_config.min_width = min_value;
            node().node.layout_config.max_width = max_value;
        }

        void height_range(float min_value, float max_value)
        {
            node().node.layout_config.min_height = min_value;
            node().node.layout_config.max_height = max_value;
        }

    private:
        [[nodiscard]] ReconciledNode& node() noexcept
        {
            assert(nodes_ != nullptr);
            assert(node_index_ < nodes_->size());
            return (*nodes_)[node_index_];
        }

        std::vector<ReconciledNode>* nodes_ = nullptr;
        std::size_t node_index_ = 0U;
        const AppBreakpoint* app_breakpoint_ = nullptr;
    };

    explicit FrameBuilder(std::vector<ReconciledNode>& nodes) noexcept
        : nodes_(nodes)
    {
    }

    void clear() noexcept
    {
        nodes_.clear();
        keys_in_frame_.clear();
    }

    FrameBuilder& app_breakpoint(AppBreakpoint breakpoint) noexcept
    {
        app_breakpoint_ = breakpoint;
        return *this;
    }

    [[nodiscard]] AppBreakpoint app_breakpoint() const noexcept
    {
        return app_breakpoint_;
    }

    void reserve(std::size_t count)
    {
        nodes_.reserve(count);
        keys_in_frame_.reserve(count);
    }

    Entry add(std::string parent_key, std::string key, NodeKind kind)
    {
        if(key.empty()) {
            throw std::invalid_argument("FrameBuilder key must not be empty.");
        }
        if(!keys_in_frame_.insert(key).second) {
            throw std::invalid_argument("FrameBuilder duplicate key: " + key);
        }

        ReconciledNode node;
        node.parent_key = std::move(parent_key);
        node.node.key = std::move(key);
        node.node.kind = kind;
        nodes_.push_back(std::move(node));
        return Entry(&nodes_, nodes_.size() - 1U, &app_breakpoint_);
    }

    Entry panel(std::string parent_key, std::string key)
    {
        return add(std::move(parent_key), std::move(key), NodeKind::Panel);
    }

    Entry view(std::string parent_key, std::string key)
    {
        return add(std::move(parent_key), std::move(key), NodeKind::View);
    }

    Entry column(std::string parent_key, std::string key, float gap = 6.0f, float padding = 8.0f)
    {
        Entry entry = view(std::move(parent_key), std::move(key));
        entry.column(gap, padding);
        return entry;
    }

    Entry column(
        std::string parent_key,
        std::string key,
        std::initializer_list<AxisTrack> tracks,
        float gap = 6.0f,
        float padding = 8.0f
    )
    {
        Entry entry = view(std::move(parent_key), std::move(key));
        entry.column(std::move(tracks), gap, padding);
        return entry;
    }

    Entry row(std::string parent_key, std::string key, float gap = 6.0f, float padding = 8.0f)
    {
        Entry entry = view(std::move(parent_key), std::move(key));
        entry.row(gap, padding);
        return entry;
    }

    Entry row(
        std::string parent_key,
        std::string key,
        std::initializer_list<AxisTrack> tracks,
        float gap = 6.0f,
        float padding = 8.0f
    )
    {
        Entry entry = view(std::move(parent_key), std::move(key));
        entry.row(std::move(tracks), gap, padding);
        return entry;
    }

    Entry grid(
        std::string parent_key,
        std::string key,
        std::size_t auto_columns = 1U,
        float gap = 6.0f,
        float padding = 8.0f
    )
    {
        Entry entry = view(std::move(parent_key), std::move(key));
        entry.grid(auto_columns, gap, padding);
        return entry;
    }

    Entry grid(
        std::string parent_key,
        std::string key,
        std::initializer_list<AxisTrack> columns,
        float gap = 6.0f,
        float padding = 8.0f
    )
    {
        Entry entry = view(std::move(parent_key), std::move(key));
        entry.grid(std::move(columns), gap, padding);
        return entry;
    }

    Entry label(std::string parent_key, std::string key, std::string text)
    {
        Entry entry = add(std::move(parent_key), std::move(key), NodeKind::Label);
        entry.text(std::move(text));
        return entry;
    }

    Entry image(std::string parent_key, std::string key, std::string image_source)
    {
        Entry entry = add(std::move(parent_key), std::move(key), NodeKind::Image);
        entry.image_source(std::move(image_source));
        return entry;
    }

    Entry image_asset(std::string parent_key, std::string key, std::string image_asset_id)
    {
        Entry entry = add(std::move(parent_key), std::move(key), NodeKind::Image);
        entry.image_asset(std::move(image_asset_id));
        return entry;
    }

    Entry button(std::string parent_key, std::string key, std::string text)
    {
        Entry entry = add(std::move(parent_key), std::move(key), NodeKind::Button);
        entry.text(std::move(text));
        return entry;
    }

    Entry text_edit(std::string parent_key, std::string key, std::string value_utf8)
    {
        Entry entry = add(std::move(parent_key), std::move(key), NodeKind::TextEdit);
        entry.text(std::move(value_utf8));
        entry.allow_ctrl_shortcuts(true);
        return entry;
    }

    Entry custom(std::string parent_key, std::string key)
    {
        return add(std::move(parent_key), std::move(key), NodeKind::Custom);
    }

private:
    std::vector<ReconciledNode>& nodes_;
    std::unordered_set<std::string> keys_in_frame_{};
    AppBreakpoint app_breakpoint_ = AppBreakpoint::Compact;
};

} // namespace guinevere::ui
