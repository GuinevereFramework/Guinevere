#pragma once

#include <string_view>

#include <guinevere/gfx/color.hpp>
#include <guinevere/gfx/rect.hpp>

namespace guinevere::gfx {

class Renderer {
public:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    Renderer(Renderer&&) = default;
    Renderer& operator=(Renderer&&) = default;

    virtual ~Renderer() = default;

    virtual void clear(Color color) = 0;
    virtual void fill_rect(Rect rect, Color color) = 0;
    virtual void stroke_rect(Rect rect, Color color, float thickness) = 0;
    virtual void push_clip(Rect rect) = 0;
    virtual void pop_clip() = 0;

    virtual bool set_font(std::string_view ttf_path, int pixel_height) = 0;
    virtual void draw_text(float x, float y, std::string_view text, Color color) = 0;
    virtual bool draw_image(
        Rect rect,
        std::string_view image_path,
        Color tint = Color{1.0f, 1.0f, 1.0f, 1.0f}
    ) = 0;
    virtual float measure_text(std::string_view text) = 0;

    virtual void present() = 0;
};

} // namespace guinevere::gfx
