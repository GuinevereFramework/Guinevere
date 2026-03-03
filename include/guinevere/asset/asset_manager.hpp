#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace guinevere::gfx {
class Renderer;
}

namespace guinevere::asset {

class AssetManager {
public:
    [[nodiscard]] static constexpr std::string_view image_uri_prefix() noexcept
    {
        return "asset://image/";
    }

    [[nodiscard]] static constexpr std::string_view font_uri_prefix() noexcept
    {
        return "asset://font/";
    }

    void add_search_path(std::filesystem::path path);
    void clear_search_paths() noexcept;

    [[nodiscard]] const std::vector<std::filesystem::path>& search_paths() const noexcept
    {
        return search_paths_;
    }

    [[nodiscard]] bool register_font(std::string id, std::filesystem::path path);
    [[nodiscard]] bool register_image(std::string id, std::filesystem::path path);

    [[nodiscard]] bool has_font(std::string_view id) const noexcept;
    [[nodiscard]] bool has_image(std::string_view id) const noexcept;

    [[nodiscard]] std::string resolve_font(std::string_view id) const;
    [[nodiscard]] std::string resolve_image(std::string_view id) const;
    [[nodiscard]] std::string resolve_reference(std::string_view value) const;

    [[nodiscard]] bool apply_font(
        gfx::Renderer& renderer,
        std::string_view font_reference,
        int pixel_height
    ) const;

    [[nodiscard]] static std::string image_uri(std::string_view id);
    [[nodiscard]] static std::string font_uri(std::string_view id);
    [[nodiscard]] static bool is_image_uri(std::string_view value) noexcept;
    [[nodiscard]] static bool is_font_uri(std::string_view value) noexcept;

private:
    [[nodiscard]] static std::string normalize_asset_id(std::string_view id);
    [[nodiscard]] std::string resolve_file_path(std::filesystem::path value) const;

    std::vector<std::filesystem::path> search_paths_;
    std::unordered_map<std::string, std::string> font_assets_;
    std::unordered_map<std::string, std::string> image_assets_;
};

} // namespace guinevere::asset
