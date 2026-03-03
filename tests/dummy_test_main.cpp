#include <string_view>
#include <vector>

#include <guinevere/ui/runtime.hpp>

namespace {

class MockRenderer final : public guinevere::gfx::Renderer {
public:
    void clear(guinevere::gfx::Color) override
    {
    }

    void fill_rect(guinevere::gfx::Rect, guinevere::gfx::Color) override
    {
        ++fill_rect_calls_;
    }

    void stroke_rect(guinevere::gfx::Rect, guinevere::gfx::Color, float) override
    {
        ++stroke_rect_calls_;
    }

    void push_clip(guinevere::gfx::Rect) override
    {
        ++push_clip_calls_;
    }

    void pop_clip() override
    {
        ++pop_clip_calls_;
    }

    bool set_font(std::string_view, int) override
    {
        return true;
    }

    void draw_text(float, float, std::string_view, guinevere::gfx::Color) override
    {
        ++draw_text_calls_;
    }

    bool draw_image(
        guinevere::gfx::Rect,
        std::string_view,
        guinevere::gfx::Color
    ) override
    {
        ++draw_image_calls_;
        return true;
    }

    float measure_text(std::string_view text) override
    {
        return static_cast<float>(text.size()) * 8.0f;
    }

    void present() override
    {
    }

    [[nodiscard]] int total_draw_calls() const noexcept
    {
        return fill_rect_calls_ + stroke_rect_calls_ + draw_text_calls_ + draw_image_calls_;
    }

private:
    int fill_rect_calls_ = 0;
    int stroke_rect_calls_ = 0;
    int draw_text_calls_ = 0;
    int draw_image_calls_ = 0;
    int push_clip_calls_ = 0;
    int pop_clip_calls_ = 0;
};

} // namespace

int main()
{
    guinevere::ui::UiTree tree;
    guinevere::ui::Pipeline pipeline;
    std::vector<guinevere::ui::ReconciledNode> frame;
    guinevere::ui::FrameBuilder builder(frame);
    MockRenderer renderer;

    builder
        .panel("root", "panel")
        .layout(guinevere::gfx::Rect{40.0f, 40.0f, 320.0f, 180.0f})
        .column(10.0f, 10.0f);
    builder.label("panel", "title", "Dirty Repaint Test");
    builder
        .image("panel", "preview", "tests/assets/sample.png")
        .height_fixed(64.0f);

    guinevere::ui::Reconciler::reconcile(tree, "root", frame);
    pipeline.layout(tree, guinevere::gfx::Rect{0.0f, 0.0f, 800.0f, 600.0f});

    if(!tree.has_any_dirty()) {
        return 1;
    }

    const auto draw_commands = pipeline.build_draw_commands(tree);
    pipeline.paint(
        renderer,
        tree,
        draw_commands,
        guinevere::gfx::Color{0.10f, 0.12f, 0.16f, 1.0f}
    );

    const int first_pass_draw_calls = renderer.total_draw_calls();
    if(first_pass_draw_calls <= 0) {
        return 1;
    }

    if(tree.has_any_dirty()) {
        return 1;
    }

    guinevere::ui::Reconciler::reconcile(tree, "root", frame);
    pipeline.layout(tree, guinevere::gfx::Rect{0.0f, 0.0f, 800.0f, 600.0f});

    if(tree.has_any_dirty()) {
        return 1;
    }

    pipeline.paint(
        renderer,
        tree,
        draw_commands,
        guinevere::gfx::Color{0.10f, 0.12f, 0.16f, 1.0f}
    );

    if(renderer.total_draw_calls() != first_pass_draw_calls) {
        return 1;
    }

    return 0;
}
