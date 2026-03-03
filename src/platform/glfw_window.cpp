#include <stdexcept>
#include <utility>

#include <GLFW/glfw3.h>

#include <guinevere/platform/glfw_window.hpp>
#include <guinevere/ui/runtime.hpp>

namespace guinevere::platform {

static int g_glfw_init_count = 0;

GlfwWindow::GlfwWindow() = default;

GlfwWindow::GlfwWindow(GlfwWindow&& other) noexcept
    : window_(other.window_)
    , owns_init_(other.owns_init_)
    , text_input_utf8_(std::move(other.text_input_utf8_))
    , backspace_presses_(other.backspace_presses_)
    , enter_presses_(other.enter_presses_)
    , left_arrow_presses_(other.left_arrow_presses_)
    , right_arrow_presses_(other.right_arrow_presses_)
    , up_arrow_presses_(other.up_arrow_presses_)
    , down_arrow_presses_(other.down_arrow_presses_)
    , toggle_caret_presses_(other.toggle_caret_presses_)
    , scroll_delta_x_(other.scroll_delta_x_)
    , scroll_delta_y_(other.scroll_delta_y_)
{
    other.window_ = nullptr;
    other.owns_init_ = false;
    other.text_input_utf8_.clear();
    other.backspace_presses_ = 0;
    other.enter_presses_ = 0;
    other.left_arrow_presses_ = 0;
    other.right_arrow_presses_ = 0;
    other.up_arrow_presses_ = 0;
    other.down_arrow_presses_ = 0;
    other.toggle_caret_presses_ = 0;
    other.scroll_delta_x_ = 0.0f;
    other.scroll_delta_y_ = 0.0f;

    if(window_ != nullptr) {
        glfwSetWindowUserPointer(window_, this);
    }
}

GlfwWindow& GlfwWindow::operator=(GlfwWindow&& other) noexcept
{
    if(this == &other) {
        return *this;
    }

    if(window_ != nullptr) {
        glfwDestroyWindow(window_);
    }

    if(owns_init_) {
        owns_init_ = false;
        --g_glfw_init_count;
        if(g_glfw_init_count <= 0) {
            g_glfw_init_count = 0;
            glfwTerminate();
        }
    }

    window_ = other.window_;
    owns_init_ = other.owns_init_;
    text_input_utf8_ = std::move(other.text_input_utf8_);
    backspace_presses_ = other.backspace_presses_;
    enter_presses_ = other.enter_presses_;
    left_arrow_presses_ = other.left_arrow_presses_;
    right_arrow_presses_ = other.right_arrow_presses_;
    up_arrow_presses_ = other.up_arrow_presses_;
    down_arrow_presses_ = other.down_arrow_presses_;
    toggle_caret_presses_ = other.toggle_caret_presses_;
    scroll_delta_x_ = other.scroll_delta_x_;
    scroll_delta_y_ = other.scroll_delta_y_;
    other.window_ = nullptr;
    other.owns_init_ = false;
    other.text_input_utf8_.clear();
    other.backspace_presses_ = 0;
    other.enter_presses_ = 0;
    other.left_arrow_presses_ = 0;
    other.right_arrow_presses_ = 0;
    other.up_arrow_presses_ = 0;
    other.down_arrow_presses_ = 0;
    other.toggle_caret_presses_ = 0;
    other.scroll_delta_x_ = 0.0f;
    other.scroll_delta_y_ = 0.0f;

    if(window_ != nullptr) {
        glfwSetWindowUserPointer(window_, this);
    }

    return *this;
}

GlfwWindow::~GlfwWindow()
{
    if(window_ != nullptr) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    if(owns_init_) {
        owns_init_ = false;
        --g_glfw_init_count;
        if(g_glfw_init_count <= 0) {
            g_glfw_init_count = 0;
            glfwTerminate();
        }
    }
}

void GlfwWindow::create(int width, int height, const char* title)
{
    if(window_ != nullptr) {
        throw std::runtime_error("GlfwWindow::create called twice");
    }

    if(g_glfw_init_count == 0) {
        if(!glfwInit()) {
            throw std::runtime_error("glfwInit failed");
        }
    }

    ++g_glfw_init_count;
    owns_init_ = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if(window_ == nullptr) {
        owns_init_ = false;
        --g_glfw_init_count;
        if(g_glfw_init_count <= 0) {
            g_glfw_init_count = 0;
            glfwTerminate();
        }
        throw std::runtime_error("glfwCreateWindow failed");
    }

    glfwSetWindowUserPointer(window_, this);
    glfwSetCharCallback(window_, &GlfwWindow::on_char);
    glfwSetKeyCallback(window_, &GlfwWindow::on_key);
    glfwSetScrollCallback(window_, &GlfwWindow::on_scroll);

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);
}

bool GlfwWindow::is_open() const
{
    return window_ != nullptr && glfwWindowShouldClose(window_) == 0;
}

void GlfwWindow::poll_events()
{
    glfwPollEvents();
}

void GlfwWindow::swap_buffers()
{
    if(window_ == nullptr) {
        return;
    }

    glfwSwapBuffers(window_);
}

void GlfwWindow::get_framebuffer_size(int& width, int& height) const
{
    width = 0;
    height = 0;

    if(window_ == nullptr) {
        return;
    }

    glfwGetFramebufferSize(window_, &width, &height);
}

void GlfwWindow::get_cursor_position(float& x, float& y) const
{
    x = 0.0f;
    y = 0.0f;

    if(window_ == nullptr) {
        return;
    }

    double cursor_x = 0.0;
    double cursor_y = 0.0;
    glfwGetCursorPos(window_, &cursor_x, &cursor_y);

    int window_w = 0;
    int window_h = 0;
    int framebuffer_w = 0;
    int framebuffer_h = 0;
    glfwGetWindowSize(window_, &window_w, &window_h);
    glfwGetFramebufferSize(window_, &framebuffer_w, &framebuffer_h);

    if(window_w > 0 && window_h > 0) {
        const float sx = static_cast<float>(framebuffer_w) / static_cast<float>(window_w);
        const float sy = static_cast<float>(framebuffer_h) / static_cast<float>(window_h);
        x = static_cast<float>(cursor_x) * sx;
        y = static_cast<float>(cursor_y) * sy;
        return;
    }

    x = static_cast<float>(cursor_x);
    y = static_cast<float>(cursor_y);
}

bool GlfwWindow::is_left_mouse_button_down() const
{
    if(window_ == nullptr) {
        return false;
    }

    return glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
}

std::string GlfwWindow::consume_text_input_utf8()
{
    std::string out = std::move(text_input_utf8_);
    text_input_utf8_.clear();
    return out;
}

unsigned int GlfwWindow::consume_backspace_presses() noexcept
{
    const unsigned int out = backspace_presses_;
    backspace_presses_ = 0;
    return out;
}

unsigned int GlfwWindow::consume_enter_presses() noexcept
{
    const unsigned int out = enter_presses_;
    enter_presses_ = 0;
    return out;
}

unsigned int GlfwWindow::consume_left_arrow_presses() noexcept
{
    const unsigned int out = left_arrow_presses_;
    left_arrow_presses_ = 0;
    return out;
}

unsigned int GlfwWindow::consume_right_arrow_presses() noexcept
{
    const unsigned int out = right_arrow_presses_;
    right_arrow_presses_ = 0;
    return out;
}

unsigned int GlfwWindow::consume_up_arrow_presses() noexcept
{
    const unsigned int out = up_arrow_presses_;
    up_arrow_presses_ = 0;
    return out;
}

unsigned int GlfwWindow::consume_down_arrow_presses() noexcept
{
    const unsigned int out = down_arrow_presses_;
    down_arrow_presses_ = 0;
    return out;
}

unsigned int GlfwWindow::consume_toggle_caret_presses() noexcept
{
    const unsigned int out = toggle_caret_presses_;
    toggle_caret_presses_ = 0;
    return out;
}

float GlfwWindow::consume_scroll_delta_x() noexcept
{
    const float out = scroll_delta_x_;
    scroll_delta_x_ = 0.0f;
    return out;
}

float GlfwWindow::consume_scroll_delta_y() noexcept
{
    const float out = scroll_delta_y_;
    scroll_delta_y_ = 0.0f;
    return out;
}

ui::InputState GlfwWindow::consume_input_state()
{
    ui::InputState input;
    get_cursor_position(input.x, input.y);
    input.scroll_x = consume_scroll_delta_x();
    input.scroll_y = consume_scroll_delta_y();
    input.left_down = is_left_mouse_button_down();
    input.text_utf8 = consume_text_input_utf8();
    input.backspace_presses = consume_backspace_presses();
    input.enter_presses = consume_enter_presses();
    input.left_arrow_presses = consume_left_arrow_presses();
    input.right_arrow_presses = consume_right_arrow_presses();
    input.up_arrow_presses = consume_up_arrow_presses();
    input.down_arrow_presses = consume_down_arrow_presses();
    input.toggle_caret_presses = consume_toggle_caret_presses();
    return input;
}

void GlfwWindow::on_char(GLFWwindow* window, unsigned int codepoint)
{
    if(window == nullptr) {
        return;
    }

    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
    if(self == nullptr) {
        return;
    }

    // Ignore control characters and let key callback handle control keys.
    if(codepoint < 0x20u || codepoint == 0x7Fu) {
        return;
    }

    self->append_utf8(codepoint);
}

void GlfwWindow::on_key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;

    if(window == nullptr) {
        return;
    }

    if(action != GLFW_PRESS && action != GLFW_REPEAT) {
        return;
    }

    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
    if(self == nullptr) {
        return;
    }

    if(key == GLFW_KEY_BACKSPACE) {
        ++self->backspace_presses_;
        return;
    }

    if(key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
        ++self->enter_presses_;
        return;
    }

    if(key == GLFW_KEY_LEFT) {
        ++self->left_arrow_presses_;
        return;
    }

    if(key == GLFW_KEY_RIGHT) {
        ++self->right_arrow_presses_;
        return;
    }

    if(key == GLFW_KEY_UP) {
        ++self->up_arrow_presses_;
        return;
    }

    if(key == GLFW_KEY_DOWN) {
        ++self->down_arrow_presses_;
        return;
    }

    if(key == GLFW_KEY_INSERT) {
        if(action == GLFW_PRESS) {
            ++self->toggle_caret_presses_;
        }
    }
}

void GlfwWindow::on_scroll(GLFWwindow* window, double xoffset, double yoffset)
{
    if(window == nullptr) {
        return;
    }

    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
    if(self == nullptr) {
        return;
    }

    self->scroll_delta_x_ += static_cast<float>(xoffset);
    self->scroll_delta_y_ += static_cast<float>(yoffset);
}

void GlfwWindow::append_utf8(unsigned int codepoint)
{
    if(codepoint > 0x10FFFFu || (codepoint >= 0xD800u && codepoint <= 0xDFFFu)) {
        return;
    }

    if(codepoint < 0x80u) {
        text_input_utf8_.push_back(static_cast<char>(codepoint));
        return;
    }

    if(codepoint < 0x800u) {
        text_input_utf8_.push_back(static_cast<char>(0xC0u | (codepoint >> 6u)));
        text_input_utf8_.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
        return;
    }

    if(codepoint < 0x10000u) {
        text_input_utf8_.push_back(static_cast<char>(0xE0u | (codepoint >> 12u)));
        text_input_utf8_.push_back(static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3Fu)));
        text_input_utf8_.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
        return;
    }

    text_input_utf8_.push_back(static_cast<char>(0xF0u | (codepoint >> 18u)));
    text_input_utf8_.push_back(static_cast<char>(0x80u | ((codepoint >> 12u) & 0x3Fu)));
    text_input_utf8_.push_back(static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3Fu)));
    text_input_utf8_.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
}

} // namespace guinevere::platform
