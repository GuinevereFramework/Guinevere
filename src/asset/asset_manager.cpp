#include <algorithm>
#include <system_error>

#include <guinevere/asset/asset_manager.hpp>
#include <guinevere/gfx/renderer.hpp>

namespace guinevere::asset {

namespace {

[[nodiscard]] std::filesystem::path normalize_path(std::filesystem::path path)
{
    if(path.empty()) {
        return {};
    }

    std::error_code ec;
    if(!path.is_absolute()) {
        path = std::filesystem::absolute(path, ec);
        if(ec) {
            ec.clear();
        }
    }

    return path.lexically_normal();
}

[[nodiscard]] bool file_exists(const std::filesystem::path& path)
{
    std::error_code ec;
    const bool exists = std::filesystem::exists(path, ec);
    if(ec || !exists) {
        return false;
    }

    ec.clear();
    return std::filesystem::is_regular_file(path, ec) && !ec;
}

[[nodiscard]] bool directory_exists(const std::filesystem::path& path)
{
    std::error_code ec;
    const bool exists = std::filesystem::exists(path, ec);
    if(ec || !exists) {
        return false;
    }

    ec.clear();
    return std::filesystem::is_directory(path, ec) && !ec;
}

[[nodiscard]] std::string to_stable_string(const std::filesystem::path& path)
{
    return path.generic_string();
}

[[nodiscard]] std::string build_uri(std::string_view prefix, std::string_view id)
{
    std::string out;
    out.reserve(prefix.size() + id.size());
    out.append(prefix);
    out.append(id);
    return out;
}

[[nodiscard]] bool map_contains(
    const std::unordered_map<std::string, std::string>& map,
    std::string_view id
) noexcept
{
    return map.find(std::string(id)) != map.end();
}

[[nodiscard]] std::string map_resolve(
    const std::unordered_map<std::string, std::string>& map,
    std::string_view id
)
{
    const auto it = map.find(std::string(id));
    if(it == map.end()) {
        return {};
    }
    return it->second;
}

[[nodiscard]] std::filesystem::path detect_asset_root()
{
    static const std::filesystem::path cached = [] {
        std::error_code ec;
        std::filesystem::path probe = std::filesystem::current_path(ec);
        if(ec || probe.empty()) {
            return std::filesystem::path{};
        }

        for(;;) {
            const std::filesystem::path asset_root = probe / "assets";
            if(directory_exists(asset_root)
                && file_exists(probe / "CMakeLists.txt")
                && directory_exists(probe / "src")
                && directory_exists(probe / "include")) {
                return asset_root.lexically_normal();
            }

            const std::filesystem::path parent = probe.parent_path();
            if(parent.empty() || parent == probe) {
                break;
            }
            probe = parent;
        }

        return std::filesystem::path{};
    }();

    return cached;
}

[[nodiscard]] std::filesystem::path resolve_guinevere_virtual_path(const std::filesystem::path& path)
{
    const std::string raw = path.generic_string();
    constexpr std::string_view prefix = "Guinevere:/";
    if(!std::string_view(raw).starts_with(prefix)) {
        return path;
    }

    const std::filesystem::path asset_root = detect_asset_root();
    if(asset_root.empty()) {
        return {};
    }

    std::filesystem::path relative(raw.substr(prefix.size()));
    if(relative.is_absolute()) {
        relative = relative.relative_path();
    }

    return asset_root / relative;
}

} // namespace

void AssetManager::add_search_path(std::filesystem::path path)
{
    if(path.empty()) {
        return;
    }

    const std::filesystem::path normalized = normalize_path(std::move(path));
    if(normalized.empty()) {
        return;
    }

    const auto existing = std::find(search_paths_.begin(), search_paths_.end(), normalized);
    if(existing == search_paths_.end()) {
        search_paths_.push_back(normalized);
    }
}

void AssetManager::clear_search_paths() noexcept
{
    search_paths_.clear();
}

bool AssetManager::register_font(std::string id, std::filesystem::path path)
{
    const std::string normalized_id = normalize_asset_id(id);
    if(normalized_id.empty()) {
        return false;
    }

    const std::string resolved = resolve_file_path(std::move(path));
    if(resolved.empty()) {
        return false;
    }

    font_assets_[normalized_id] = resolved;
    return true;
}

bool AssetManager::register_image(std::string id, std::filesystem::path path)
{
    const std::string normalized_id = normalize_asset_id(id);
    if(normalized_id.empty()) {
        return false;
    }

    const std::string resolved = resolve_file_path(std::move(path));
    if(resolved.empty()) {
        return false;
    }

    image_assets_[normalized_id] = resolved;
    return true;
}

bool AssetManager::has_font(std::string_view id) const noexcept
{
    return map_contains(font_assets_, id);
}

bool AssetManager::has_image(std::string_view id) const noexcept
{
    return map_contains(image_assets_, id);
}

std::string AssetManager::resolve_font(std::string_view id) const
{
    return map_resolve(font_assets_, id);
}

std::string AssetManager::resolve_image(std::string_view id) const
{
    return map_resolve(image_assets_, id);
}

std::string AssetManager::resolve_reference(std::string_view value) const
{
    if(value.empty()) {
        return {};
    }

    if(is_image_uri(value)) {
        return resolve_image(value.substr(image_uri_prefix().size()));
    }

    if(is_font_uri(value)) {
        return resolve_font(value.substr(font_uri_prefix().size()));
    }

    return std::string(value);
}

bool AssetManager::apply_font(
    gfx::Renderer& renderer,
    std::string_view font_reference,
    int pixel_height
) const
{
    if(font_reference.empty() || pixel_height <= 0) {
        return false;
    }

    std::string resolved_font_path;
    if(is_font_uri(font_reference)) {
        resolved_font_path = resolve_reference(font_reference);
    } else {
        resolved_font_path = resolve_font(font_reference);
        if(resolved_font_path.empty()) {
            resolved_font_path = resolve_file_path(std::filesystem::path(std::string(font_reference)));
        }
    }

    if(resolved_font_path.empty()) {
        resolved_font_path = std::string(font_reference);
    }

    return renderer.set_font(resolved_font_path, pixel_height);
}

std::string AssetManager::image_uri(std::string_view id)
{
    return build_uri(image_uri_prefix(), id);
}

std::string AssetManager::font_uri(std::string_view id)
{
    return build_uri(font_uri_prefix(), id);
}

bool AssetManager::is_image_uri(std::string_view value) noexcept
{
    return value.starts_with(image_uri_prefix());
}

bool AssetManager::is_font_uri(std::string_view value) noexcept
{
    return value.starts_with(font_uri_prefix());
}

std::string AssetManager::normalize_asset_id(std::string_view id)
{
    return std::string(id);
}

std::string AssetManager::resolve_file_path(std::filesystem::path value) const
{
    if(value.empty()) {
        return {};
    }

    value = resolve_guinevere_virtual_path(value);
    if(value.empty()) {
        return {};
    }

    const auto try_candidate = [](const std::filesystem::path& candidate) -> std::string {
        const std::filesystem::path normalized = normalize_path(candidate);
        if(normalized.empty()) {
            return {};
        }

        if(!file_exists(normalized)) {
            return {};
        }

        return to_stable_string(normalized);
    };

    if(value.is_absolute()) {
        return try_candidate(value);
    }

    if(const std::string direct = try_candidate(value); !direct.empty()) {
        return direct;
    }

    for(const std::filesystem::path& search_path : search_paths_) {
        if(search_path.empty()) {
            continue;
        }

        const std::filesystem::path candidate = search_path / value;
        if(const std::string resolved = try_candidate(candidate); !resolved.empty()) {
            return resolved;
        }
    }

    return {};
}

} // namespace guinevere::asset
