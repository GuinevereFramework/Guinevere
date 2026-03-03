#pragma once

#include <string>

struct GLFWwindow;

namespace guinevere::ui {
struct InputState;
}

namespace guinevere::platform {

class GlfwWindow {
public:
    GlfwWindow();
    GlfwWindow(const GlfwWindow&) = delete;
    GlfwWindow& operator=(const GlfwWindow&) = delete;

    GlfwWindow(GlfwWindow&& other) noexcept;
    GlfwWindow& operator=(GlfwWindow&& other) noexcept;

    ~GlfwWindow();

    void create(int width, int height, const char* title);
    bool is_open() const;
    void poll_events();
    void swap_buffers();
    void get_framebuffer_size(int& width, int& height) const;
    void get_cursor_position(float& x, float& y) const;
    bool is_left_mouse_button_down() const;
    [[nodiscard]] std::string consume_text_input_utf8();
    [[nodiscard]] unsigned int consume_backspace_presses() noexcept;
    [[nodiscard]] unsigned int consume_enter_presses() noexcept;
    [[nodiscard]] unsigned int consume_left_arrow_presses() noexcept;
    [[nodiscard]] unsigned int consume_right_arrow_presses() noexcept;
    [[nodiscard]] unsigned int consume_up_arrow_presses() noexcept;
    [[nodiscard]] unsigned int consume_down_arrow_presses() noexcept;
    [[nodiscard]] unsigned int consume_toggle_caret_presses() noexcept;
    [[nodiscard]] float consume_scroll_delta_x() noexcept;
    [[nodiscard]] float consume_scroll_delta_y() noexcept;
    [[nodiscard]] ui::InputState consume_input_state();

private:
    static void on_char(GLFWwindow* window, unsigned int codepoint);
    static void on_key(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void on_scroll(GLFWwindow* window, double xoffset, double yoffset);
    void append_utf8(unsigned int codepoint);

    GLFWwindow* window_ = nullptr;
    bool owns_init_ = false;
    std::string text_input_utf8_;
    unsigned int backspace_presses_ = 0;
    unsigned int enter_presses_ = 0;
    unsigned int left_arrow_presses_ = 0;
    unsigned int right_arrow_presses_ = 0;
    unsigned int up_arrow_presses_ = 0;
    unsigned int down_arrow_presses_ = 0;
    unsigned int toggle_caret_presses_ = 0;
    float scroll_delta_x_ = 0.0f;
    float scroll_delta_y_ = 0.0f;
};

} // namespace guinevere::platform
