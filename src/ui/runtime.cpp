#include <utility>

#include <guinevere/app/runner.hpp>
#include <guinevere/asset/asset_manager.hpp>
#include <guinevere/ui/runtime.hpp>

namespace guinevere::ui {

UiRuntime::UiRuntime(std::string root_key)
    : frame_builder_(frame_)
    , root_key_(std::move(root_key))
{
    if(root_key_.empty()) {
        throw std::invalid_argument("UiRuntime root_key must not be empty.");
    }
}

UiRuntime::FrameScope::FrameScope(
    UiRuntime& runtime,
    app::Context& context,
    std::optional<gfx::Color> clear_color
) noexcept
    : runtime_(&runtime)
    , context_(&context)
    , clear_color_(clear_color)
{
}

UiRuntime::FrameScope::FrameScope(FrameScope&& other) noexcept
    : runtime_(other.runtime_)
    , context_(other.context_)
    , clear_color_(other.clear_color_)
    , ended_(other.ended_)
{
    other.runtime_ = nullptr;
    other.context_ = nullptr;
    other.ended_ = true;
}

UiRuntime::FrameScope& UiRuntime::FrameScope::operator=(FrameScope&& other) noexcept
{
    if(this == &other) {
        return *this;
    }

    if(!ended_) {
        try {
            (void)end();
        } catch(...) {
        }
    }

    runtime_ = other.runtime_;
    context_ = other.context_;
    clear_color_ = other.clear_color_;
    ended_ = other.ended_;

    other.runtime_ = nullptr;
    other.context_ = nullptr;
    other.ended_ = true;
    return *this;
}

UiRuntime::FrameScope::~FrameScope()
{
    if(!ended_) {
        try {
            (void)end();
        } catch(...) {
        }
    }
}

FrameBuilder& UiRuntime::FrameScope::frame_builder() noexcept
{
    return runtime_->frame_builder();
}

StateStore& UiRuntime::FrameScope::state_store() noexcept
{
    return runtime_->state_store();
}

ComponentScope UiRuntime::FrameScope::root_component(std::string component_key_prefix)
{
    return runtime_->root_component(std::move(component_key_prefix));
}

app::Context& UiRuntime::FrameScope::context() noexcept
{
    return *context_;
}

bool UiRuntime::FrameScope::end()
{
    if(ended_ || runtime_ == nullptr || context_ == nullptr) {
        ended_ = true;
        return false;
    }

    ended_ = true;
    return runtime_->end_frame(*context_, clear_color_);
}

gfx::Rect UiRuntime::viewport(app::Context& context) const noexcept
{
    int framebuffer_w = 0;
    int framebuffer_h = 0;
    context.window.get_framebuffer_size(framebuffer_w, framebuffer_h);
    return gfx::Rect{
        0.0f,
        0.0f,
        static_cast<float>(framebuffer_w),
        static_cast<float>(framebuffer_h)
    };
}

AppScaffoldResult UiRuntime::resolve_scaffold(
    app::Context& context,
    const AppScaffoldSpec& spec
) noexcept
{
    const AppScaffoldResult scaffold = resolve_app_scaffold(viewport(context), spec);
    frame_builder_.app_breakpoint(scaffold.app_layout.breakpoint);
    return scaffold;
}

void UiRuntime::begin_frame(InputState input) noexcept
{
    interactions_.begin_frame(std::move(input));
    frame_builder_.clear();
}

void UiRuntime::begin_frame(app::Context& context) noexcept
{
    const gfx::Rect fb_viewport = viewport(context);
    frame_builder_.app_breakpoint(detect_app_breakpoint(fb_viewport.w));
    begin_frame(context.window.consume_input_state());
}

UiRuntime::FrameScope UiRuntime::frame(
    app::Context& context,
    std::optional<gfx::Color> clear_color
) noexcept
{
    begin_frame(context);
    return FrameScope(*this, context, clear_color);
}

bool UiRuntime::end_frame(
    gfx::Renderer& renderer,
    gfx::Rect viewport,
    std::optional<gfx::Color> clear_color
)
{
    return end_frame(renderer, viewport, static_cast<const asset::AssetManager*>(nullptr), clear_color);
}

bool UiRuntime::end_frame(
    gfx::Renderer& renderer,
    gfx::Rect viewport,
    const asset::AssetManager& assets,
    std::optional<gfx::Color> clear_color
)
{
    return end_frame(renderer, viewport, &assets, clear_color);
}

bool UiRuntime::end_frame(
    app::Context& context,
    std::optional<gfx::Color> clear_color
)
{
    const gfx::Rect fb_viewport = viewport(context);
    return end_frame(context.renderer, fb_viewport, context.assets, clear_color);
}

bool UiRuntime::end_frame(
    gfx::Renderer& renderer,
    gfx::Rect viewport,
    const asset::AssetManager* assets,
    std::optional<gfx::Color> clear_color
)
{
    Reconciler::reconcile(tree_, root_key_, frame_);
    pipeline_.layout(tree_, viewport);

    bool has_scroll_update = false;
    const std::size_t node_count = tree_.nodes().size();
    for(std::size_t i = 0U; i < node_count; ++i) {
        UiNode* node = tree_.get(i);
        if(node != nullptr && interactions_.update_scroll_container(*node)) {
            has_scroll_update = true;
        }
    }
    if(has_scroll_update) {
        pipeline_.layout(tree_, viewport);
    }

    const bool has_interaction = interactions_.update_frame(tree_, frame_, renderer);
    interactions_.end_frame();

    const auto commands = pipeline_.build_draw_commands(tree_);
    if(clear_color.has_value()) {
        if(assets != nullptr) {
            pipeline_.paint(renderer, tree_, commands, *assets, clear_color);
        } else {
            pipeline_.paint(renderer, tree_, commands, clear_color);
        }
    } else {
        if(assets != nullptr) {
            pipeline_.paint(renderer, commands, *assets);
        } else {
            pipeline_.paint(renderer, commands);
        }
        tree_.clear_all_dirty(DirtyFlags::All);
        tree_.clear_dirty_regions();
    }

    return has_interaction || has_scroll_update;
}

} // namespace guinevere::ui
