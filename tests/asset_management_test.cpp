#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <guinevere/asset/asset_manager.hpp>
#include <guinevere/ui/runtime.hpp>

namespace {

class MockRenderer final : public guinevere::gfx::Renderer {
public:
    void clear(guinevere::gfx::Color) override
    {
    }

    void fill_rect(guinevere::gfx::Rect, guinevere::gfx::Color) override
    {
    }

    void stroke_rect(guinevere::gfx::Rect, guinevere::gfx::Color, float) override
    {
    }

    void push_clip(guinevere::gfx::Rect) override
    {
    }

    void pop_clip() override
    {
    }

    bool set_font(std::string_view ttf_path, int) override
    {
        last_font_path = std::string(ttf_path);
        return true;
    }

    void draw_text(float, float, std::string_view, guinevere::gfx::Color) override
    {
    }

    bool draw_image(
        guinevere::gfx::Rect,
        std::string_view image_path,
        guinevere::gfx::Color
    ) override
    {
        ++draw_image_calls;
        last_image_path = std::string(image_path);
        return true;
    }

    float measure_text(std::string_view text) override
    {
        return static_cast<float>(text.size()) * 8.0f;
    }

    void present() override
    {
    }

    int draw_image_calls = 0;
    std::string last_image_path;
    std::string last_font_path;
};

[[nodiscard]] std::filesystem::path create_temp_dir()
{
    std::error_code ec;
    const std::filesystem::path temp_root = std::filesystem::temp_directory_path(ec);
    if(ec || temp_root.empty()) {
        return {};
    }

    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path dir = temp_root / ("guinevere_asset_test_" + std::to_string(stamp));
    std::filesystem::create_directories(dir, ec);
    if(ec) {
        return {};
    }

    return dir;
}

[[nodiscard]] bool write_dummy_file(const std::filesystem::path& path)
{
    std::ofstream out(path, std::ios::binary);
    if(!out) {
        return false;
    }

    out.put('\0');
    return static_cast<bool>(out);
}

struct TempDirGuard {
    TempDirGuard()
        : path(create_temp_dir())
    {
    }

    ~TempDirGuard()
    {
        if(path.empty()) {
            return;
        }

        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }

    std::filesystem::path path;
};

} // namespace

int main()
{
    TempDirGuard temp_dir_guard;
    if(temp_dir_guard.path.empty()) {
        return 1;
    }

    const std::filesystem::path font_dir = temp_dir_guard.path / "fonts";
    const std::filesystem::path image_dir = temp_dir_guard.path / "images";
    const std::filesystem::path font_file = font_dir / "ui.ttf";
    const std::filesystem::path image_file = image_dir / "hero.png";

    std::error_code ec;
    std::filesystem::create_directories(font_dir, ec);
    if(ec) {
        return 1;
    }
    std::filesystem::create_directories(image_dir, ec);
    if(ec) {
        return 1;
    }
    if(!write_dummy_file(font_file) || !write_dummy_file(image_file)) {
        return 1;
    }

    guinevere::asset::AssetManager assets;
    assets.add_search_path(temp_dir_guard.path);

    if(!assets.register_font("ui", "fonts/ui.ttf")) {
        return 1;
    }
    if(!assets.register_image("hero", "images/hero.png")) {
        return 1;
    }

    if(!assets.has_font("ui") || !assets.has_image("hero")) {
        return 1;
    }

    const std::string resolved_font = assets.resolve_font("ui");
    const std::string resolved_image = assets.resolve_image("hero");
    if(resolved_font.empty() || resolved_image.empty()) {
        return 1;
    }

    if(assets.resolve_reference(guinevere::asset::AssetManager::font_uri("ui")) != resolved_font) {
        return 1;
    }
    if(assets.resolve_reference(guinevere::asset::AssetManager::image_uri("hero")) != resolved_image) {
        return 1;
    }

    MockRenderer renderer;
    if(!assets.apply_font(renderer, "ui", 18)) {
        return 1;
    }
    if(renderer.last_font_path != resolved_font) {
        return 1;
    }

    guinevere::ui::DrawCommand image_command;
    image_command.type = guinevere::ui::DrawCommandType::Image;
    image_command.rect = guinevere::gfx::Rect{20.0f, 20.0f, 100.0f, 80.0f};
    image_command.color = guinevere::gfx::Color{1.0f, 1.0f, 1.0f, 1.0f};
    image_command.image_source = guinevere::asset::AssetManager::image_uri("hero");

    std::vector<guinevere::ui::DrawCommand> commands;
    commands.push_back(image_command);

    guinevere::ui::Pipeline pipeline;
    pipeline.paint(renderer, commands, assets);
    if(renderer.draw_image_calls != 1) {
        return 1;
    }
    if(renderer.last_image_path != resolved_image) {
        return 1;
    }

    image_command.image_source = guinevere::asset::AssetManager::image_uri("missing");
    commands[0] = image_command;
    pipeline.paint(renderer, commands, assets);
    if(renderer.draw_image_calls != 1) {
        return 1;
    }

    return 0;
}
