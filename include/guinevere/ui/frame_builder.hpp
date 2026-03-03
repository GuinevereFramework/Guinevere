#pragma once

#include <guinevere/ui/types.hpp>

namespace guinevere::ui {

class FrameBuilder {
public:
    class Entry {
    public:
        explicit Entry(ReconciledNode* node, const AppBreakpoint* app_breakpoint) noexcept
            : node_(node)
            , app_breakpoint_(app_breakpoint)
        {
        }

        Entry& text(std::string value)
        {
            node_->node.text = std::move(value);
            return *this;
        }

        Entry& image_source(std::string value)
        {
            node_->node.image_source = std::move(value);
            return *this;
        }

        Entry& image_asset(std::string asset_id)
        {
            constexpr std::string_view prefix = "asset://image/";
            node_->node.image_source.clear();
            node_->node.image_source.reserve(prefix.size() + asset_id.size());
            node_->node.image_source.append(prefix);
            node_->node.image_source.append(asset_id);
            return *this;
        }

        Entry& image_tint(gfx::Color value)
        {
            node_->node.image_tint = value;
            return *this;
        }

        Entry& classes(std::vector<std::string> values)
        {
            node_->node.classes = std::move(values);
            return *this;
        }

        template<typename OnClick>
        Entry& on_click(OnClick&& on_click)
        {
            node_->node.on_click = std::forward<OnClick>(on_click);
            return *this;
        }

        template<typename OnChange>
        Entry& on_text_change(OnChange&& on_change)
        {
            node_->node.on_text_change = std::forward<OnChange>(on_change);
            return *this;
        }

        template<typename OnSubmit>
        Entry& on_text_submit(OnSubmit&& on_submit)
        {
            node_->node.on_text_submit = std::forward<OnSubmit>(on_submit);
            return *this;
        }

        Entry& allow_caret_toggle(bool value = true)
        {
            node_->node.allow_user_toggle_caret = value;
            return *this;
        }

        Entry& allow_arrow_left(bool value = true)
        {
            node_->node.allow_arrow_left = value;
            return *this;
        }

        Entry& allow_arrow_right(bool value = true)
        {
            node_->node.allow_arrow_right = value;
            return *this;
        }

        Entry& allow_arrow_up(bool value = true)
        {
            node_->node.allow_arrow_up = value;
            return *this;
        }

        Entry& allow_arrow_down(bool value = true)
        {
            node_->node.allow_arrow_down = value;
            return *this;
        }

        Entry& allow_arrow_keys(bool value = true)
        {
            node_->node.allow_arrow_left = value;
            node_->node.allow_arrow_right = value;
            node_->node.allow_arrow_up = value;
            node_->node.allow_arrow_down = value;
            return *this;
        }

        Entry& allow_mouse_set_cursor(bool value = true)
        {
            node_->node.allow_mouse_set_cursor = value;
            return *this;
        }

        Entry& layout(gfx::Rect rect)
        {
            node_->node.layout_hint = rect;
            return *this;
        }

        Entry& column(float gap = 6.0f, float padding = 8.0f)
        {
            node_->node.layout_config.direction = LayoutDirection::Column;
            node_->node.layout_config.gap = gap;
            node_->node.layout_config.padding = padding;
            return *this;
        }

        Entry& row(float gap = 6.0f, float padding = 8.0f)
        {
            node_->node.layout_config.direction = LayoutDirection::Row;
            node_->node.layout_config.gap = gap;
            node_->node.layout_config.padding = padding;
            return *this;
        }

        Entry& align(AlignItems value)
        {
            node_->node.layout_config.align_items = value;
            return *this;
        }

        Entry& justify(JustifyContent value)
        {
            node_->node.layout_config.justify_content = value;
            return *this;
        }

        Entry& overflow(OverflowPolicy value)
        {
            node_->node.layout_config.overflow = value;
            return *this;
        }

        Entry& overflow_clip()
        {
            return overflow(OverflowPolicy::Clip);
        }

        Entry& overflow_visible()
        {
            return overflow(OverflowPolicy::Visible);
        }

        Entry& overflow_wrap()
        {
            return overflow(OverflowPolicy::Wrap);
        }

        Entry& overflow_scroll()
        {
            return overflow(OverflowPolicy::Scroll);
        }

        Entry& align_start()
        {
            return align(AlignItems::Start);
        }

        Entry& align_center()
        {
            return align(AlignItems::Center);
        }

        Entry& align_end()
        {
            return align(AlignItems::End);
        }

        Entry& align_stretch()
        {
            return align(AlignItems::Stretch);
        }

        Entry& justify_start()
        {
            return justify(JustifyContent::Start);
        }

        Entry& justify_center()
        {
            return justify(JustifyContent::Center);
        }

        Entry& justify_end()
        {
            return justify(JustifyContent::End);
        }

        Entry& justify_space_between()
        {
            return justify(JustifyContent::SpaceBetween);
        }

        Entry& width_fill(float weight = 1.0f)
        {
            node_->node.layout_config.width_mode = SizeMode::Fill;
            node_->node.layout_config.width_fill_weight = weight;
            return *this;
        }

        Entry& height_fill(float weight = 1.0f)
        {
            node_->node.layout_config.height_mode = SizeMode::Fill;
            node_->node.layout_config.height_fill_weight = weight;
            return *this;
        }

        Entry& width_fixed(float width)
        {
            node_->node.layout_config.width_mode = SizeMode::Fixed;
            node_->node.layout_config.fixed_width = width;
            return *this;
        }

        Entry& width(float width)
        {
            return width_fixed(width);
        }

        Entry& width(const ResponsiveProperty& value)
        {
            const AppBreakpoint breakpoint = app_breakpoint_ != nullptr
                ? *app_breakpoint_
                : AppBreakpoint::Compact;
            return width_fixed(resolve_responsive_property(breakpoint, value));
        }

        Entry& height_fixed(float height)
        {
            node_->node.layout_config.height_mode = SizeMode::Fixed;
            node_->node.layout_config.fixed_height = height;
            return *this;
        }

        Entry& height(float height)
        {
            return height_fixed(height);
        }

        Entry& height(const ResponsiveProperty& value)
        {
            const AppBreakpoint breakpoint = app_breakpoint_ != nullptr
                ? *app_breakpoint_
                : AppBreakpoint::Compact;
            return height_fixed(resolve_responsive_property(breakpoint, value));
        }

        Entry& size_fixed(float width, float height)
        {
            node_->node.layout_config.width_mode = SizeMode::Fixed;
            node_->node.layout_config.height_mode = SizeMode::Fixed;
            node_->node.layout_config.fixed_width = width;
            node_->node.layout_config.fixed_height = height;
            return *this;
        }

        Entry& size_auto()
        {
            node_->node.layout_config.width_mode = SizeMode::Auto;
            node_->node.layout_config.height_mode = SizeMode::Auto;
            return *this;
        }

        Entry& min_width(float value)
        {
            node_->node.layout_config.min_width = value;
            return *this;
        }

        Entry& max_width(float value)
        {
            node_->node.layout_config.max_width = value;
            return *this;
        }

        Entry& min_height(float value)
        {
            node_->node.layout_config.min_height = value;
            return *this;
        }

        Entry& max_height(float value)
        {
            node_->node.layout_config.max_height = value;
            return *this;
        }

        Entry& width_range(float min_value, float max_value)
        {
            node_->node.layout_config.min_width = min_value;
            node_->node.layout_config.max_width = max_value;
            return *this;
        }

        Entry& height_range(float min_value, float max_value)
        {
            node_->node.layout_config.min_height = min_value;
            node_->node.layout_config.max_height = max_value;
            return *this;
        }

    private:
        ReconciledNode* node_ = nullptr;
        const AppBreakpoint* app_breakpoint_ = nullptr;
    };

    explicit FrameBuilder(std::vector<ReconciledNode>& nodes) noexcept
        : nodes_(nodes)
    {
    }

    void clear() noexcept
    {
        nodes_.clear();
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
    }

    Entry add(std::string parent_key, std::string key, NodeKind kind)
    {
        ReconciledNode node;
        node.parent_key = std::move(parent_key);
        node.node.key = std::move(key);
        node.node.kind = kind;
        nodes_.push_back(std::move(node));
        return Entry(&nodes_.back(), &app_breakpoint_);
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
        return view(std::move(parent_key), std::move(key)).column(gap, padding);
    }

    Entry row(std::string parent_key, std::string key, float gap = 6.0f, float padding = 8.0f)
    {
        return view(std::move(parent_key), std::move(key)).row(gap, padding);
    }

    Entry label(std::string parent_key, std::string key, std::string text)
    {
        return add(std::move(parent_key), std::move(key), NodeKind::Label).text(std::move(text));
    }

    Entry image(std::string parent_key, std::string key, std::string image_source)
    {
        return add(std::move(parent_key), std::move(key), NodeKind::Image)
            .image_source(std::move(image_source));
    }

    Entry image_asset(std::string parent_key, std::string key, std::string image_asset_id)
    {
        return add(std::move(parent_key), std::move(key), NodeKind::Image)
            .image_asset(std::move(image_asset_id));
    }

    Entry button(std::string parent_key, std::string key, std::string text)
    {
        return add(std::move(parent_key), std::move(key), NodeKind::Button).text(std::move(text));
    }

    Entry text_edit(std::string parent_key, std::string key, std::string value_utf8)
    {
        return add(std::move(parent_key), std::move(key), NodeKind::TextEdit).text(std::move(value_utf8));
    }

    Entry custom(std::string parent_key, std::string key)
    {
        return add(std::move(parent_key), std::move(key), NodeKind::Custom);
    }

private:
    std::vector<ReconciledNode>& nodes_;
    AppBreakpoint app_breakpoint_ = AppBreakpoint::Compact;
};

} // namespace guinevere::ui
