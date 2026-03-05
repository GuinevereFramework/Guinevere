#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include <guinevere/ui/runtime.hpp>

namespace guinevere::ui {

namespace {

struct NodeSnapshot {
    NodeKind kind = NodeKind::View;
    std::string parent_key;
    gfx::Rect layout{};
    NodeProps props{};
    LayoutConfig layout_config{};
    NodeState state{};
    bool has_explicit_layout = false;
};

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

[[nodiscard]] std::unordered_map<std::string, NodeSnapshot> snapshot_nodes(const UiTree& tree)
{
    std::unordered_map<std::string, NodeSnapshot> nodes;
    nodes.reserve(tree.nodes().size());

    for(const UiNode& node : tree.nodes()) {
        NodeSnapshot snapshot;
        snapshot.kind = node.kind;
        snapshot.layout = node.layout;
        snapshot.props = node.props;
        snapshot.layout_config = node.layout_config;
        snapshot.state = node.state;
        snapshot.has_explicit_layout = node.has_explicit_layout;

        if(node.parent != UiNode::npos) {
            const UiNode* parent = tree.get(node.parent);
            if(parent != nullptr) {
                snapshot.parent_key = parent->key;
            }
        }

        nodes[node.key] = std::move(snapshot);
    }

    return nodes;
}

[[nodiscard]] bool layout_config_equal(const LayoutConfig& lhs, const LayoutConfig& rhs) noexcept
{
    const auto tracks_equal = [](
                                  const std::vector<LayoutConfig::AxisTrackConstraint>& lhs_tracks,
                                  const std::vector<LayoutConfig::AxisTrackConstraint>& rhs_tracks
                              ) noexcept {
        if(lhs_tracks.size() != rhs_tracks.size()) {
            return false;
        }

        for(std::size_t i = 0U; i < lhs_tracks.size(); ++i) {
            const auto& lhs_track = lhs_tracks[i];
            const auto& rhs_track = rhs_tracks[i];
            if(lhs_track.min_size != rhs_track.min_size
                || lhs_track.preferred_size != rhs_track.preferred_size
                || lhs_track.max_size != rhs_track.max_size
                || lhs_track.grow_weight != rhs_track.grow_weight
                || lhs_track.shrink_priority != rhs_track.shrink_priority) {
                return false;
            }
        }
        return true;
    };

    if(!tracks_equal(lhs.main_axis_tracks, rhs.main_axis_tracks)
        || !tracks_equal(lhs.grid_columns, rhs.grid_columns)
        || !tracks_equal(lhs.grid_rows, rhs.grid_rows)) {
        return false;
    }

    return lhs.direction == rhs.direction
        && lhs.gap == rhs.gap
        && lhs.padding == rhs.padding
        && lhs.align_items == rhs.align_items
        && lhs.justify_content == rhs.justify_content
        && lhs.overflow == rhs.overflow
        && lhs.width_mode == rhs.width_mode
        && lhs.height_mode == rhs.height_mode
        && lhs.width_fill_weight == rhs.width_fill_weight
        && lhs.height_fill_weight == rhs.height_fill_weight
        && lhs.min_width == rhs.min_width
        && lhs.max_width == rhs.max_width
        && lhs.min_height == rhs.min_height
        && lhs.max_height == rhs.max_height
        && lhs.fixed_width == rhs.fixed_width
        && lhs.fixed_height == rhs.fixed_height
        && lhs.grid_auto_columns == rhs.grid_auto_columns;
}

[[nodiscard]] bool node_props_equal(const NodeProps& lhs, const NodeProps& rhs) noexcept
{
    return lhs.text == rhs.text
        && lhs.image_source == rhs.image_source
        && lhs.image_tint.r == rhs.image_tint.r
        && lhs.image_tint.g == rhs.image_tint.g
        && lhs.image_tint.b == rhs.image_tint.b
        && lhs.image_tint.a == rhs.image_tint.a
        && lhs.classes == rhs.classes
        && lhs.allow_user_toggle_caret == rhs.allow_user_toggle_caret
        && lhs.allow_arrow_left == rhs.allow_arrow_left
        && lhs.allow_arrow_right == rhs.allow_arrow_right
        && lhs.allow_arrow_up == rhs.allow_arrow_up
        && lhs.allow_arrow_down == rhs.allow_arrow_down
        && lhs.allow_mouse_set_cursor == rhs.allow_mouse_set_cursor;
}

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

void mark_ancestor_chain_dirty(UiTree& tree, std::size_t index, DirtyFlags flags)
{
    const UiNode* node = tree.get(index);
    while(node != nullptr && node->parent != UiNode::npos) {
        tree.mark_dirty(node->parent, flags);
        node = tree.get(node->parent);
    }
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

} // namespace

void UiTree::clear() noexcept
{
    nodes_.clear();
    by_key_.clear();
    dirty_regions_.clear();
    root_ = npos;
}

std::size_t UiTree::create_root(std::string key)
{
    clear();

    if(key.empty()) {
        return npos;
    }

    UiNode root_node;
    root_node.key = std::move(key);
    root_node.kind = NodeKind::Root;
    root_node.parent = npos;
    root_node.previous_layout = root_node.layout;
    root_node.dirty_flags = DirtyFlags::All;

    nodes_.push_back(std::move(root_node));
    root_ = 0;
    by_key_[nodes_.front().key] = root_;

    return root_;
}

std::size_t UiTree::append_child(std::size_t parent, UiNode node)
{
    if(!is_valid(parent) || node.key.empty()) {
        return npos;
    }

    const auto existing_it = by_key_.find(node.key);
    if(existing_it != by_key_.end()) {
        return existing_it->second;
    }

    node.parent = parent;
    node.children.clear();

    const std::size_t index = nodes_.size();
    nodes_.push_back(std::move(node));
    nodes_[parent].children.push_back(index);
    by_key_[nodes_[index].key] = index;

    return index;
}

bool UiTree::is_valid(std::size_t index) const noexcept
{
    return index < nodes_.size();
}

std::size_t UiTree::root() const noexcept
{
    return root_;
}

const UiNode* UiTree::get(std::size_t index) const noexcept
{
    if(!is_valid(index)) {
        return nullptr;
    }

    return &nodes_[index];
}

UiNode* UiTree::get(std::size_t index) noexcept
{
    if(!is_valid(index)) {
        return nullptr;
    }

    return &nodes_[index];
}

std::size_t UiTree::find(std::string_view key) const
{
    const auto it = by_key_.find(std::string(key));
    if(it == by_key_.end()) {
        return npos;
    }

    return it->second;
}

const std::vector<UiNode>& UiTree::nodes() const noexcept
{
    return nodes_;
}

const std::vector<gfx::Rect>& UiTree::dirty_regions() const noexcept
{
    return dirty_regions_;
}

bool UiTree::has_any_dirty(DirtyFlags flags) const noexcept
{
    if(has_flag(flags, DirtyFlags::Paint) && !dirty_regions_.empty()) {
        return true;
    }

    for(const UiNode& node : nodes_) {
        if(has_flag(node.dirty_flags, flags)) {
            return true;
        }
    }

    return false;
}

void UiTree::mark_dirty(std::size_t index, DirtyFlags flags)
{
    UiNode* node = get(index);
    if(node == nullptr) {
        return;
    }
    node->dirty_flags |= flags;
}

void UiTree::mark_subtree_dirty(std::size_t index, DirtyFlags flags)
{
    UiNode* node = get(index);
    if(node == nullptr) {
        return;
    }

    node->dirty_flags |= flags;
    for(const std::size_t child_index : node->children) {
        mark_subtree_dirty(child_index, flags);
    }
}

void UiTree::clear_all_dirty(DirtyFlags flags)
{
    const DirtyFlags clear_mask = flags;
    const bool clear_paint = has_flag(clear_mask, DirtyFlags::Paint);
    for(UiNode& node : nodes_) {
        if(clear_paint) {
            node.previous_layout = node.layout;
        }
        node.dirty_flags = static_cast<DirtyFlags>(
            static_cast<std::uint32_t>(node.dirty_flags)
            & ~static_cast<std::uint32_t>(clear_mask)
        );
    }
}

void UiTree::invalidate(gfx::Rect rect)
{
    if(rect_is_empty(rect)) {
        return;
    }

    gfx::Rect merged = rect;
    std::size_t i = 0U;
    while(i < dirty_regions_.size()) {
        if(rect_intersects(dirty_regions_[i], merged)) {
            merged = rect_union(dirty_regions_[i], merged);
            dirty_regions_.erase(dirty_regions_.begin() + static_cast<std::ptrdiff_t>(i));
            i = 0U;
            continue;
        }
        ++i;
    }

    dirty_regions_.push_back(merged);
}

void UiTree::clear_dirty_regions() noexcept
{
    dirty_regions_.clear();
}

void Reconciler::reconcile(
    UiTree& tree,
    std::string_view root_key,
    const std::vector<ReconciledNode>& nodes
)
{
    if(root_key.empty()) {
        return;
    }

    const auto previous_nodes = snapshot_nodes(tree);
    std::unordered_set<std::string> seen_keys;
    seen_keys.reserve(nodes.size() + 1U);

    const std::string root_string(root_key);
    const std::size_t root_index = tree.create_root(root_string);
    if(root_index == UiTree::npos) {
        return;
    }

    tree.clear_dirty_regions();

    UiNode* root = tree.get(root_index);
    if(root != nullptr) {
        root->dirty_flags = DirtyFlags::Structure | DirtyFlags::Layout | DirtyFlags::Paint;
        root->previous_layout = root->layout;

        const auto root_previous_it = previous_nodes.find(root->key);
        if(root_previous_it != previous_nodes.end()) {
            root->layout = root_previous_it->second.layout;
            root->previous_layout = root_previous_it->second.layout;
            root->state = root_previous_it->second.state;
            root->dirty_flags = DirtyFlags::None;
        }
    }

    std::unordered_map<std::string, std::size_t> created;
    created[root_string] = root_index;
    seen_keys.insert(root_string);

    for(const ReconciledNode& entry : nodes) {
        if(entry.node.key.empty()) {
            continue;
        }
        if(entry.node.key == root_string) {
            throw std::invalid_argument(
                "Reconciler node key collides with root key: " + entry.node.key
            );
        }
        if(!seen_keys.insert(entry.node.key).second) {
            throw std::invalid_argument(
                "Reconciler duplicate node key in frame: " + entry.node.key
            );
        }

        const std::string resolved_parent_key = entry.parent_key.empty()
            ? root_string
            : entry.parent_key;

        const auto parent_it = created.find(resolved_parent_key);
        if(parent_it == created.end()) {
            throw std::invalid_argument(
                "Reconciler parent key not found: "
                + resolved_parent_key
                + " (child: "
                + entry.node.key
                + ")"
            );
        }
        const std::size_t parent = parent_it->second;

        UiNode node;
        node.key = entry.node.key;
        node.kind = entry.node.kind;
        node.layout_config = entry.node.layout_config;
        node.props.text = entry.node.text;
        node.props.image_source = entry.node.image_source;
        node.props.image_tint = entry.node.image_tint;
        node.props.classes = entry.node.classes;
        node.props.allow_user_toggle_caret = entry.node.allow_user_toggle_caret;
        node.props.allow_arrow_left = entry.node.allow_arrow_left;
        node.props.allow_arrow_right = entry.node.allow_arrow_right;
        node.props.allow_arrow_up = entry.node.allow_arrow_up;
        node.props.allow_arrow_down = entry.node.allow_arrow_down;
        node.props.allow_mouse_set_cursor = entry.node.allow_mouse_set_cursor;
        node.dirty_flags = DirtyFlags::Structure
            | DirtyFlags::Layout
            | DirtyFlags::Paint
            | DirtyFlags::Resource;
        node.previous_layout = node.layout;

        const auto previous_it = previous_nodes.find(node.key);
        const bool has_previous = previous_it != previous_nodes.end();
        if(has_previous) {
            const NodeSnapshot& previous = previous_it->second;
            node.state = previous.state;
            node.layout = previous.layout;
            node.previous_layout = previous.layout;
            node.has_explicit_layout = previous.has_explicit_layout;
            node.dirty_flags = DirtyFlags::None;

            if(previous.kind != node.kind) {
                node.dirty_flags |= DirtyFlags::Structure
                    | DirtyFlags::Layout
                    | DirtyFlags::Paint
                    | DirtyFlags::Resource;
            }

            if(previous.parent_key != resolved_parent_key) {
                node.dirty_flags |= DirtyFlags::Structure | DirtyFlags::Layout | DirtyFlags::Paint;
            }

            if(!layout_config_equal(previous.layout_config, node.layout_config)) {
                node.dirty_flags |= DirtyFlags::Layout | DirtyFlags::Paint;
            }

            if(!node_props_equal(previous.props, node.props)) {
                node.dirty_flags |= DirtyFlags::Paint;
                if(previous.props.text != node.props.text
                    || previous.props.classes != node.props.classes
                    || previous.props.image_source != node.props.image_source) {
                    node.dirty_flags |= DirtyFlags::Resource;
                }
            }
        }

        if(entry.node.layout_hint.has_value()) {
            node.layout = *entry.node.layout_hint;
            node.has_explicit_layout = true;
            if(!has_previous || !rect_equal(node.layout, node.previous_layout)) {
                node.dirty_flags |= DirtyFlags::Layout | DirtyFlags::Paint;
            }
        } else {
            if(has_previous && node.has_explicit_layout) {
                node.dirty_flags |= DirtyFlags::Layout | DirtyFlags::Paint;
            }
            node.has_explicit_layout = false;
        }

        const std::size_t index = tree.append_child(parent, std::move(node));
        if(index != UiTree::npos) {
            UiNode* stored = tree.get(index);
            if(stored != nullptr) {
                created[stored->key] = index;

                if(has_flag(stored->dirty_flags, DirtyFlags::Layout)) {
                    mark_ancestor_chain_dirty(
                        tree,
                        index,
                        DirtyFlags::Layout | DirtyFlags::Paint | DirtyFlags::Structure
                    );

                    if(stored->has_explicit_layout) {
                        tree.invalidate(rect_union(stored->previous_layout, stored->layout));
                    }
                } else if(has_flag(stored->dirty_flags, DirtyFlags::Paint)) {
                    tree.invalidate(stored->layout);
                }
            }
        }
    }

    for(const auto& kv : previous_nodes) {
        if(seen_keys.find(kv.first) != seen_keys.end()) {
            continue;
        }
        tree.invalidate(kv.second.layout);
        tree.mark_dirty(root_index, DirtyFlags::Paint | DirtyFlags::Structure | DirtyFlags::Layout);
    }

    const UiNode* refreshed_root = tree.get(root_index);
    if(refreshed_root != nullptr && has_flag(refreshed_root->dirty_flags, DirtyFlags::Paint)) {
        tree.invalidate(refreshed_root->layout);
    }
}


} // namespace guinevere::ui
