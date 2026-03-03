#pragma once

#include <any>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <guinevere/gfx/color.hpp>
#include <guinevere/gfx/rect.hpp>
#include <guinevere/gfx/renderer.hpp>

namespace guinevere::asset {
class AssetManager;
}

namespace guinevere::app {
struct Context;
}

namespace guinevere::ui {

enum class NodeKind {
    Root,
    Panel,
    View,
    Image,
    Label,
    Button,
    TextEdit,
    Custom,
};

enum class LayoutDirection {
    None,
    Column,
    Row,
};

enum class SizeMode {
    Auto,
    Fill,
    Fixed,
};

enum class AlignItems {
    Start,
    Center,
    End,
    Stretch,
};

enum class JustifyContent {
    Start,
    Center,
    End,
    SpaceBetween,
};

enum class OverflowPolicy {
    Clip,
    Visible,
    Wrap,
    Scroll,
};

enum class DirtyFlags : std::uint32_t {
    None = 0u,
    Structure = 1u << 0u,
    Layout = 1u << 1u,
    Paint = 1u << 2u,
    Interaction = 1u << 3u,
    Resource = 1u << 4u,
    All = (1u << 0u) | (1u << 1u) | (1u << 2u) | (1u << 3u) | (1u << 4u)
};

[[nodiscard]] constexpr DirtyFlags operator|(DirtyFlags lhs, DirtyFlags rhs) noexcept
{
    return static_cast<DirtyFlags>(
        static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs)
    );
}

[[nodiscard]] constexpr DirtyFlags operator&(DirtyFlags lhs, DirtyFlags rhs) noexcept
{
    return static_cast<DirtyFlags>(
        static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs)
    );
}

constexpr DirtyFlags& operator|=(DirtyFlags& lhs, DirtyFlags rhs) noexcept
{
    lhs = lhs | rhs;
    return lhs;
}

constexpr DirtyFlags& operator&=(DirtyFlags& lhs, DirtyFlags rhs) noexcept
{
    lhs = lhs & rhs;
    return lhs;
}

[[nodiscard]] constexpr bool has_flag(DirtyFlags value, DirtyFlags flag) noexcept
{
    return static_cast<std::uint32_t>(value & flag) != 0u;
}

struct LayoutConfig {
    LayoutDirection direction = LayoutDirection::None;
    float gap = 6.0f;
    float padding = 8.0f;
    AlignItems align_items = AlignItems::Start;
    JustifyContent justify_content = JustifyContent::Start;
    OverflowPolicy overflow = OverflowPolicy::Clip;
    SizeMode width_mode = SizeMode::Auto;
    SizeMode height_mode = SizeMode::Auto;
    float width_fill_weight = 1.0f;
    float height_fill_weight = 1.0f;
    float min_width = 0.0f;
    float max_width = 0.0f;
    float min_height = 0.0f;
    float max_height = 0.0f;
    float fixed_width = 0.0f;
    float fixed_height = 0.0f;
};

enum class AppBreakpoint {
    Compact,
    Medium,
    Expanded,
};

[[nodiscard]] const char* app_breakpoint_label(AppBreakpoint breakpoint) noexcept;

struct ResponsiveScalar {
    float compact = 0.0f;
    float medium = 0.0f;
    float expanded = 0.0f;
};

struct ResponsiveProperty {
    std::optional<float> compact = std::nullopt;
    std::optional<float> medium = std::nullopt;
    std::optional<float> expanded = std::nullopt;
};

struct EdgeInsets {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

struct ViewportLayoutConstraints {
    float margin_left = 0.0f;
    float margin_top = 0.0f;
    float margin_right = 0.0f;
    float margin_bottom = 0.0f;
    float min_width = 0.0f;
    float max_width = 0.0f;
    float min_height = 0.0f;
    float max_height = 0.0f;
    float fill_viewport_width_below = 0.0f;
    float fill_viewport_height_below = 0.0f;
    bool clamp_to_viewport = true;
};

struct AppLayoutSpec {
    float medium_breakpoint_min_width = 840.0f;
    float expanded_breakpoint_min_width = 1240.0f;
    ResponsiveScalar margin_left = ResponsiveScalar{16.0f, 28.0f, 40.0f};
    ResponsiveScalar margin_top = ResponsiveScalar{84.0f, 96.0f, 108.0f};
    ResponsiveScalar margin_right = ResponsiveScalar{16.0f, 28.0f, 40.0f};
    ResponsiveScalar margin_bottom = ResponsiveScalar{20.0f, 26.0f, 32.0f};
    EdgeInsets safe_area{};
    float min_width = 0.0f;
    float max_width = 0.0f;
    float min_height = 0.0f;
    float max_height = 0.0f;
    float fill_viewport_width_below = 0.0f;
    float fill_viewport_height_below = 0.0f;
    bool clamp_to_viewport = true;
};

struct AppLayoutResult {
    AppBreakpoint breakpoint = AppBreakpoint::Compact;
    gfx::Rect viewport{};
    gfx::Rect container{};
};

struct AppScaffoldSpec {
    AppLayoutSpec app_layout{};
    ResponsiveScalar header_height = ResponsiveScalar{34.0f, 36.0f, 38.0f};
    ResponsiveScalar header_gap = ResponsiveScalar{10.0f, 12.0f, 14.0f};
};

struct AppScaffoldResult {
    AppLayoutResult app_layout{};
    gfx::Rect header{};
    gfx::Rect body{};
};

[[nodiscard]] AppBreakpoint detect_app_breakpoint(
    float viewport_width,
    float medium_breakpoint_min_width = 840.0f,
    float expanded_breakpoint_min_width = 1240.0f
) noexcept;

[[nodiscard]] float resolve_responsive_scalar(
    AppBreakpoint breakpoint,
    const ResponsiveScalar& value
) noexcept;

[[nodiscard]] float resolve_responsive_property(
    AppBreakpoint breakpoint,
    const ResponsiveProperty& value,
    float fallback = 0.0f
) noexcept;

[[nodiscard]] AppLayoutResult resolve_app_layout(
    gfx::Rect viewport,
    const AppLayoutSpec& spec
) noexcept;

[[nodiscard]] AppScaffoldResult resolve_app_scaffold(
    gfx::Rect viewport,
    const AppScaffoldSpec& spec
) noexcept;

[[nodiscard]] gfx::Rect layout_in_viewport(
    gfx::Rect viewport,
    const ViewportLayoutConstraints& constraints
) noexcept;

[[nodiscard]] float axis_available_size(
    float container_size,
    float padding = 0.0f,
    float reserved_size = 0.0f,
    float gap = 0.0f,
    std::size_t gap_count = 0U
) noexcept;

[[nodiscard]] float resolve_axis_size(
    float preferred_size,
    float min_size = 0.0f,
    float max_size = 0.0f
) noexcept;

struct RectSplit {
    gfx::Rect start{};
    gfx::Rect end{};
};

[[nodiscard]] gfx::Rect inset_rect(gfx::Rect rect, float inset) noexcept;
[[nodiscard]] gfx::Rect inset_rect(gfx::Rect rect, const EdgeInsets& insets) noexcept;

[[nodiscard]] RectSplit split_row_start(gfx::Rect rect, float start_width, float gap = 0.0f) noexcept;
[[nodiscard]] RectSplit split_row_end(gfx::Rect rect, float end_width, float gap = 0.0f) noexcept;
[[nodiscard]] RectSplit split_column_start(gfx::Rect rect, float start_height, float gap = 0.0f) noexcept;
[[nodiscard]] RectSplit split_column_end(gfx::Rect rect, float end_height, float gap = 0.0f) noexcept;

[[nodiscard]] RectSplit split_row_ratio_start(
    gfx::Rect rect,
    float start_ratio,
    float gap = 0.0f,
    float min_size = 0.0f,
    float max_size = 0.0f
) noexcept;
[[nodiscard]] RectSplit split_row_ratio_end(
    gfx::Rect rect,
    float end_ratio,
    float gap = 0.0f,
    float min_size = 0.0f,
    float max_size = 0.0f
) noexcept;
[[nodiscard]] RectSplit split_column_ratio_start(
    gfx::Rect rect,
    float start_ratio,
    float gap = 0.0f,
    float min_size = 0.0f,
    float max_size = 0.0f
) noexcept;
[[nodiscard]] RectSplit split_column_ratio_end(
    gfx::Rect rect,
    float end_ratio,
    float gap = 0.0f,
    float min_size = 0.0f,
    float max_size = 0.0f
) noexcept;

struct NodeProps {
    std::string text;
    std::string image_source;
    gfx::Color image_tint{1.0f, 1.0f, 1.0f, 1.0f};
    std::vector<std::string> classes;
    bool allow_user_toggle_caret = true;
    bool allow_arrow_left = true;
    bool allow_arrow_right = true;
    bool allow_arrow_up = true;
    bool allow_arrow_down = true;
    bool allow_mouse_set_cursor = true;
};

struct NodeState {
    bool hovered = false;
    bool pressed = false;
    bool focused = false;
    bool disabled = false;
    float scroll_x = 0.0f;
    float scroll_y = 0.0f;
    std::size_t text_cursor = 0;
    bool caret_visible = true;
};

struct InputState {
    float x = 0.0f;
    float y = 0.0f;
    float scroll_x = 0.0f;
    float scroll_y = 0.0f;
    bool left_down = false;
    std::string text_utf8;
    unsigned int backspace_presses = 0;
    unsigned int enter_presses = 0;
    unsigned int left_arrow_presses = 0;
    unsigned int right_arrow_presses = 0;
    unsigned int up_arrow_presses = 0;
    unsigned int down_arrow_presses = 0;
    unsigned int toggle_caret_presses = 0;
};

struct UiNode {
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    std::string key;
    NodeKind kind = NodeKind::View;
    gfx::Rect layout{};
    gfx::Rect previous_layout{};
    bool has_explicit_layout = false;
    LayoutConfig layout_config{};
    NodeProps props{};
    NodeState state{};
    DirtyFlags dirty_flags = DirtyFlags::All;
    std::size_t parent = npos;
    std::vector<std::size_t> children;
};

class UiTree {
public:
    static constexpr std::size_t npos = UiNode::npos;

    void clear() noexcept;
    std::size_t create_root(std::string key = "root");
    std::size_t append_child(std::size_t parent, UiNode node);

    [[nodiscard]] bool is_valid(std::size_t index) const noexcept;
    [[nodiscard]] std::size_t root() const noexcept;

    [[nodiscard]] const UiNode* get(std::size_t index) const noexcept;
    [[nodiscard]] UiNode* get(std::size_t index) noexcept;

    [[nodiscard]] std::size_t find(std::string_view key) const;
    [[nodiscard]] const std::vector<UiNode>& nodes() const noexcept;
    [[nodiscard]] const std::vector<gfx::Rect>& dirty_regions() const noexcept;
    [[nodiscard]] bool has_any_dirty(DirtyFlags flags = DirtyFlags::All) const noexcept;
    void mark_dirty(std::size_t index, DirtyFlags flags = DirtyFlags::All);
    void mark_subtree_dirty(std::size_t index, DirtyFlags flags = DirtyFlags::All);
    void clear_all_dirty(DirtyFlags flags = DirtyFlags::All);
    void invalidate(gfx::Rect rect);
    void clear_dirty_regions() noexcept;

private:
    std::vector<UiNode> nodes_;
    std::unordered_map<std::string, std::size_t> by_key_;
    std::size_t root_ = npos;
    std::vector<gfx::Rect> dirty_regions_;
};

struct NodeDescriptor {
    std::string key;
    NodeKind kind = NodeKind::View;
    std::string text;
    std::string image_source;
    gfx::Color image_tint{1.0f, 1.0f, 1.0f, 1.0f};
    std::vector<std::string> classes;
    std::function<void()> on_click;
    std::function<void(const std::string&)> on_text_change;
    std::function<void(const std::string&)> on_text_submit;
    bool allow_user_toggle_caret = true;
    bool allow_arrow_left = true;
    bool allow_arrow_right = true;
    bool allow_arrow_up = true;
    bool allow_arrow_down = true;
    bool allow_mouse_set_cursor = true;
    std::optional<gfx::Rect> layout_hint;
    LayoutConfig layout_config{};
};

struct ReconciledNode {
    std::string parent_key;
    NodeDescriptor node;
};

class Reconciler {
public:
    static void reconcile(
        UiTree& tree,
        std::string_view root_key,
        const std::vector<ReconciledNode>& nodes
    );
};

} // namespace guinevere::ui
