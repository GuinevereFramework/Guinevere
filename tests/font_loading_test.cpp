#include <array>
#include <filesystem>
#include <iostream>
#include <string>
#include <guinevere/gfx/renderer_factory.hpp>
#include <guinevere/platform/glfw_window.hpp>

namespace {
std::string find_project_roboto()
{
    const std::array<const char*, 9> candidates = {{
        "../../../../assets/fonts/Google/Roboto-Regular.ttf",
        "../../../assets/fonts/Google/Roboto-Regular.ttf",
        "../../assets/fonts/Google/Roboto-Regular.ttf",
        "../assets/fonts/Google/Roboto-Regular.ttf",
        "assets/fonts/Google/Roboto-Regular.ttf",
        "./assets/fonts/Google/Roboto-Regular.ttf",
        "../../../../../assets/fonts/Google/Roboto-Regular.ttf",
        "../../../../../../assets/fonts/Google/Roboto-Regular.ttf",
        "D:/Cpp/project/Guinevere/assets/fonts/Google/Roboto-Regular.ttf"
    }};

    for(const char* candidate : candidates) {
        if(std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return {};
}
}

int main()
{
    try {
        guinevere::platform::GlfwWindow window;
        window.create(320, 240, "Guinevere Font Loading Test");

        auto renderer = guinevere::gfx::create_renderer(guinevere::gfx::Backend::OpenGL, window);

        struct FontCase {
            const char* label;
            std::string path;
            int pixel_height;
            bool expected;
        };

        const std::array<FontCase, 7> font_tests = {{
            {"Guinevere virtual Roboto", "Guinevere:/fonts/Google/Roboto-Regular.ttf", 24, true},
            {"Project Roboto Regular", find_project_roboto(), 24, true},
            {"Windows Segoe UI", "C:/Windows/Fonts/segoeui.ttf", 24, true},
            {"Windows Tahoma", "C:/Windows/Fonts/tahoma.ttf", 24, true},
            {"Windows Arial", "C:/Windows/Fonts/arial.ttf", 24, true},
            {"Windows Times New Roman", "C:/Windows/Fonts/times.ttf", 24, true},
            {"Invalid path", "../../../fonts/not-found.ttf", 24, false}
        }};

        int failed = 0;
        for(const FontCase& item : font_tests) {
            const bool loaded = renderer->set_font(item.path, item.pixel_height);
            if(loaded != item.expected) {
                ++failed;
                std::cerr << "[FAIL] " << item.label << " expected=" << item.expected
                          << " actual=" << loaded << " path=" << item.path << '\n';
            }
        }

        if(failed != 0) {
            return 1;
        }

        return 0;
    } catch(const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return 1;
    }
}
