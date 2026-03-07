#pragma once

#include <guinevere/ui/types.hpp>

namespace guinevere::ui {

class InteractionState {
public:
    void begin_frame(InputState input) noexcept;
    bool update_frame(UiTree& tree, const std::vector<ReconciledNode>& frame, gfx::Renderer* renderer = nullptr);
    bool update_frame(UiTree& tree, const std::vector<ReconciledNode>& frame, gfx::Renderer& renderer)
    {
        return update_frame(tree, frame, &renderer);
    }
    bool update_scroll_container(UiNode& node, float pixels_per_scroll_step = 48.0f);
    bool update_button(UiNode& node);
    bool update_text_edit(
        UiNode& node,
        std::string& value_utf8,
        bool* submitted = nullptr,
        gfx::Renderer* renderer = nullptr
    );

    bool update_text_edit(UiNode& node, std::string& value_utf8, gfx::Renderer& renderer, bool* submitted = nullptr)
    {
        return update_text_edit(node, value_utf8, submitted, &renderer);
    }

    template<typename OnClick>
    bool update_button(UiNode& node, OnClick&& on_click)
    {
        const bool clicked = update_button(node);
        if(clicked) {
            std::invoke(std::forward<OnClick>(on_click));
        }
        return clicked;
    }

    template<typename OnChange>
    bool update_text_edit(UiNode& node, std::string& value_utf8, OnChange&& on_change)
    {
        const bool changed = update_text_edit(node, value_utf8, nullptr, nullptr);
        if(changed) {
            std::invoke(std::forward<OnChange>(on_change), value_utf8);
        }
        return changed;
    }

    template<typename OnChange>
    bool update_text_edit(
        UiNode& node,
        std::string& value_utf8,
        gfx::Renderer& renderer,
        OnChange&& on_change
    )
    {
        const bool changed = update_text_edit(node, value_utf8, nullptr, &renderer);
        if(changed) {
            std::invoke(std::forward<OnChange>(on_change), value_utf8);
        }
        return changed;
    }

    template<typename OnChange, typename OnSubmit>
    bool update_text_edit(
        UiNode& node,
        std::string& value_utf8,
        OnChange&& on_change,
        OnSubmit&& on_submit
    )
    {
        bool submitted = false;
        const bool changed = update_text_edit(node, value_utf8, &submitted, nullptr);
        if(changed) {
            std::invoke(std::forward<OnChange>(on_change), value_utf8);
        }
        if(submitted) {
            std::invoke(std::forward<OnSubmit>(on_submit), value_utf8);
        }
        return changed;
    }

    template<typename OnChange, typename OnSubmit>
    bool update_text_edit(
        UiNode& node,
        std::string& value_utf8,
        gfx::Renderer& renderer,
        OnChange&& on_change,
        OnSubmit&& on_submit
    )
    {
        bool submitted = false;
        const bool changed = update_text_edit(node, value_utf8, &submitted, &renderer);
        if(changed) {
            std::invoke(std::forward<OnChange>(on_change), value_utf8);
        }
        if(submitted) {
            std::invoke(std::forward<OnSubmit>(on_submit), value_utf8);
        }
        return changed;
    }

    void end_frame() noexcept;

private:
    InputState input_{};
    bool mouse_was_down_ = false;
    bool pressed_edge_ = false;
    bool released_edge_ = false;
    std::unordered_map<std::string, bool> press_armed_;
    std::string active_text_edit_key_;
    std::string clipboard_utf8_;
};

enum class DrawCommandType {
    FillRect,
    StrokeRect,
    Text,
    TextInput,
    Image,
    PushClip,
    PopClip,
};

struct DrawCommand {
    DrawCommandType type = DrawCommandType::FillRect;
    gfx::Rect rect{};
    gfx::Color color{};
    float thickness = 1.0f;
    float text_x = 0.0f;
    float text_y = 0.0f;
    std::string text;
    bool focused = false;
    std::size_t cursor_index = 0;
    bool caret_visible = true;
    std::size_t selection_anchor_index = 0;
    TextEditInputType text_edit_input_type = TextEditInputType::SingleLine;
    std::size_t max_lines = 1U;
    bool has_selection_background_color = false;
    gfx::Color selection_background_color{0.02f, 0.02f, 0.02f, 0.88f};
    bool has_selection_text_color = false;
    gfx::Color selection_text_color{0.94f, 0.97f, 1.0f, 1.0f};
    std::string image_source;
};

class Pipeline {
public:
    void layout(UiTree& tree, gfx::Rect viewport) const;
    void layout(UiTree& tree, gfx::Rect viewport, gfx::Renderer* renderer) const;
    void layout(UiTree& tree, gfx::Rect viewport, gfx::Renderer& renderer) const
    {
        layout(tree, viewport, &renderer);
    }
    std::vector<DrawCommand> build_draw_commands(const UiTree& tree) const;
    void paint(gfx::Renderer& renderer, const std::vector<DrawCommand>& commands) const;
    void paint(
        gfx::Renderer& renderer,
        const std::vector<DrawCommand>& commands,
        const asset::AssetManager& assets
    ) const;
    void paint(
        gfx::Renderer& renderer,
        UiTree& tree,
        const std::vector<DrawCommand>& commands,
        std::optional<gfx::Color> clear_color = std::nullopt
    ) const;
    void paint(
        gfx::Renderer& renderer,
        UiTree& tree,
        const std::vector<DrawCommand>& commands,
        const asset::AssetManager& assets,
        std::optional<gfx::Color> clear_color = std::nullopt
    ) const;

private:
    void paint(
        gfx::Renderer& renderer,
        const std::vector<DrawCommand>& commands,
        const asset::AssetManager* assets
    ) const;
    void paint(
        gfx::Renderer& renderer,
        UiTree& tree,
        const std::vector<DrawCommand>& commands,
        const asset::AssetManager* assets,
        std::optional<gfx::Color> clear_color
    ) const;
};

} // namespace guinevere::ui
