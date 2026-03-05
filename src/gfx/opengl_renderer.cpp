#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <filesystem>
#include <limits>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cstdint>
#include <guinevere/gfx/renderer.hpp>

#include <guinevere/text/freetype_library.hpp>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <gdiplus.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <guinevere/platform/glfw_window.hpp>

namespace guinevere::gfx {

namespace {

struct Utf8DecodeResult {
    std::uint32_t codepoint = 0;
    std::size_t bytes_consumed = 0;
};

Utf8DecodeResult decode_one_utf8(std::string_view text, std::size_t i)
{
    if(i >= text.size()) {
        return Utf8DecodeResult{0u, 0u};
    }

    const auto b0 = static_cast<unsigned char>(text[i]);
    if(b0 < 0x80u) {
        return Utf8DecodeResult{static_cast<std::uint32_t>(b0), 1u};
    }

    const auto invalid = Utf8DecodeResult{0xFFFDu, 1u};

    if((b0 & 0xE0u) == 0xC0u) {
        if(i + 1u >= text.size()) {
            return invalid;
        }
        const auto b1 = static_cast<unsigned char>(text[i + 1u]);
        if((b1 & 0xC0u) != 0x80u) {
            return invalid;
        }
        const std::uint32_t cp = ((static_cast<std::uint32_t>(b0) & 0x1Fu) << 6u)
            | (static_cast<std::uint32_t>(b1) & 0x3Fu);
        if(cp < 0x80u) {
            return invalid;
        }
        return Utf8DecodeResult{cp, 2u};
    }

    if((b0 & 0xF0u) == 0xE0u) {
        if(i + 2u >= text.size()) {
            return invalid;
        }
        const auto b1 = static_cast<unsigned char>(text[i + 1u]);
        const auto b2 = static_cast<unsigned char>(text[i + 2u]);
        if(((b1 & 0xC0u) != 0x80u) || ((b2 & 0xC0u) != 0x80u)) {
            return invalid;
        }
        const std::uint32_t cp = ((static_cast<std::uint32_t>(b0) & 0x0Fu) << 12u)
            | ((static_cast<std::uint32_t>(b1) & 0x3Fu) << 6u)
            | (static_cast<std::uint32_t>(b2) & 0x3Fu);
        if(cp < 0x800u) {
            return invalid;
        }
        if(cp >= 0xD800u && cp <= 0xDFFFu) {
            return invalid;
        }
        return Utf8DecodeResult{cp, 3u};
    }

    if((b0 & 0xF8u) == 0xF0u) {
        if(i + 3u >= text.size()) {
            return invalid;
        }
        const auto b1 = static_cast<unsigned char>(text[i + 1u]);
        const auto b2 = static_cast<unsigned char>(text[i + 2u]);
        const auto b3 = static_cast<unsigned char>(text[i + 3u]);
        if(((b1 & 0xC0u) != 0x80u) || ((b2 & 0xC0u) != 0x80u) || ((b3 & 0xC0u) != 0x80u)) {
            return invalid;
        }
        const std::uint32_t cp = ((static_cast<std::uint32_t>(b0) & 0x07u) << 18u)
            | ((static_cast<std::uint32_t>(b1) & 0x3Fu) << 12u)
            | ((static_cast<std::uint32_t>(b2) & 0x3Fu) << 6u)
            | (static_cast<std::uint32_t>(b3) & 0x3Fu);
        if(cp < 0x10000u || cp > 0x10FFFFu) {
            return invalid;
        }
        return Utf8DecodeResult{cp, 4u};
    }

    return invalid;
}

[[nodiscard]] bool is_valid_utf8(std::string_view text)
{
    std::size_t i = 0U;
    while(i < text.size()) {
        const unsigned char b0 = static_cast<unsigned char>(text[i]);
        if(b0 < 0x80u) {
            ++i;
            continue;
        }

        if((b0 & 0xE0u) == 0xC0u) {
            if(i + 1U >= text.size()) {
                return false;
            }
            const unsigned char b1 = static_cast<unsigned char>(text[i + 1U]);
            if((b1 & 0xC0u) != 0x80u) {
                return false;
            }
            const std::uint32_t cp = ((static_cast<std::uint32_t>(b0) & 0x1Fu) << 6u)
                | (static_cast<std::uint32_t>(b1) & 0x3Fu);
            if(cp < 0x80u) {
                return false;
            }
            i += 2U;
            continue;
        }

        if((b0 & 0xF0u) == 0xE0u) {
            if(i + 2U >= text.size()) {
                return false;
            }
            const unsigned char b1 = static_cast<unsigned char>(text[i + 1U]);
            const unsigned char b2 = static_cast<unsigned char>(text[i + 2U]);
            if(((b1 & 0xC0u) != 0x80u) || ((b2 & 0xC0u) != 0x80u)) {
                return false;
            }
            const std::uint32_t cp = ((static_cast<std::uint32_t>(b0) & 0x0Fu) << 12u)
                | ((static_cast<std::uint32_t>(b1) & 0x3Fu) << 6u)
                | (static_cast<std::uint32_t>(b2) & 0x3Fu);
            if(cp < 0x800u || (cp >= 0xD800u && cp <= 0xDFFFu)) {
                return false;
            }
            i += 3U;
            continue;
        }

        if((b0 & 0xF8u) == 0xF0u) {
            if(i + 3U >= text.size()) {
                return false;
            }
            const unsigned char b1 = static_cast<unsigned char>(text[i + 1U]);
            const unsigned char b2 = static_cast<unsigned char>(text[i + 2U]);
            const unsigned char b3 = static_cast<unsigned char>(text[i + 3U]);
            if(((b1 & 0xC0u) != 0x80u)
                || ((b2 & 0xC0u) != 0x80u)
                || ((b3 & 0xC0u) != 0x80u)) {
                return false;
            }
            const std::uint32_t cp = ((static_cast<std::uint32_t>(b0) & 0x07u) << 18u)
                | ((static_cast<std::uint32_t>(b1) & 0x3Fu) << 12u)
                | ((static_cast<std::uint32_t>(b2) & 0x3Fu) << 6u)
                | (static_cast<std::uint32_t>(b3) & 0x3Fu);
            if(cp < 0x10000u || cp > 0x10FFFFu) {
                return false;
            }
            i += 4U;
            continue;
        }

        return false;
    }

    return true;
}

#ifdef _WIN32
[[nodiscard]] std::string convert_ansi_to_utf8(std::string_view text)
{
    if(text.empty()) {
        return {};
    }

    const int src_len = static_cast<int>(text.size());
    int wide_len = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, text.data(), src_len, nullptr, 0);
    if(wide_len <= 0) {
        wide_len = MultiByteToWideChar(CP_ACP, 0, text.data(), src_len, nullptr, 0);
    }
    if(wide_len <= 0) {
        return std::string(text);
    }

    std::wstring wide(static_cast<std::size_t>(wide_len), L'\0');
    if(MultiByteToWideChar(CP_ACP, 0, text.data(), src_len, wide.data(), wide_len) <= 0) {
        return std::string(text);
    }

    const int utf8_len = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.data(),
        wide_len,
        nullptr,
        0,
        nullptr,
        nullptr
    );
    if(utf8_len <= 0) {
        return std::string(text);
    }

    std::string utf8(static_cast<std::size_t>(utf8_len), '\0');
    if(WideCharToMultiByte(
           CP_UTF8,
           0,
           wide.data(),
           wide_len,
           utf8.data(),
           utf8_len,
           nullptr,
           nullptr
       )
        <= 0) {
        return std::string(text);
    }

    return utf8;
}
#endif

struct NormalizedUtf8Text {
    std::string converted;
    std::string_view view;
};

[[nodiscard]] NormalizedUtf8Text normalize_text_utf8(std::string_view text)
{
#ifdef _WIN32
    if(!is_valid_utf8(text)) {
        NormalizedUtf8Text normalized;
        normalized.converted = convert_ansi_to_utf8(text);
        normalized.view = normalized.converted;
        return normalized;
    }
#endif
    return NormalizedUtf8Text{{}, text};
}

[[nodiscard]] bool path_exists(const std::filesystem::path& path)
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

[[nodiscard]] std::filesystem::path normalize_path(std::filesystem::path path)
{
    if(path.empty()) {
        return {};
    }

    std::error_code ec;
    if(!path.is_absolute()) {
        path = std::filesystem::absolute(path, ec);
        if(ec) {
            return {};
        }
    }

    return path.lexically_normal();
}

[[nodiscard]] std::filesystem::path locate_guinevere_asset_root()
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
                && path_exists(probe / "CMakeLists.txt")
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

[[nodiscard]] std::string resolve_guinevere_path(std::string_view value)
{
    if(value.empty()) {
        return {};
    }

    constexpr std::string_view prefix = "Guinevere:/";
    if(!value.starts_with(prefix)) {
        return std::string(value);
    }

    const std::filesystem::path asset_root = locate_guinevere_asset_root();
    if(asset_root.empty()) {
        return std::string(value);
    }

    std::filesystem::path relative(std::string(value.substr(prefix.size())));
    if(relative.is_absolute()) {
        relative = relative.relative_path();
    }

    const std::filesystem::path resolved = normalize_path(asset_root / relative);
    if(resolved.empty()) {
        return std::string(value);
    }

    return resolved.generic_string();
}

bool select_unicode_charmap(FT_Face face)
{
    if(face == nullptr) {
        return false;
    }

    auto set_first_matching = [face](FT_UShort platform_id, FT_UShort encoding_id) -> bool {
        for(int i = 0; i < face->num_charmaps; ++i) {
            FT_CharMap cmap = face->charmaps[i];
            if(cmap == nullptr) {
                continue;
            }
            if(cmap->encoding != FT_ENCODING_UNICODE) {
                continue;
            }
            if(cmap->platform_id == platform_id && cmap->encoding_id == encoding_id) {
                return FT_Set_Charmap(face, cmap) == 0;
            }
        }
        return false;
    };

    // Prefer Windows Unicode full repertoire, then BMP.
    if(set_first_matching(3u, 10u)) {
        return true;
    }
    if(set_first_matching(3u, 1u)) {
        return true;
    }

    // Apple Unicode charmaps as next-best fallback.
    for(int i = 0; i < face->num_charmaps; ++i) {
        FT_CharMap cmap = face->charmaps[i];
        if(cmap == nullptr) {
            continue;
        }
        if(cmap->encoding != FT_ENCODING_UNICODE) {
            continue;
        }
        if(cmap->platform_id == 0u) {
            if(FT_Set_Charmap(face, cmap) == 0) {
                return true;
            }
        }
    }

    // Any Unicode charmap as last resort.
    for(int i = 0; i < face->num_charmaps; ++i) {
        FT_CharMap cmap = face->charmaps[i];
        if(cmap != nullptr && cmap->encoding == FT_ENCODING_UNICODE) {
            if(FT_Set_Charmap(face, cmap) == 0) {
                return true;
            }
        }
    }

    if(FT_Select_Charmap(face, FT_ENCODING_UNICODE) == 0) {
        return true;
    }

    return false;
}

#ifdef _WIN32
class GdiPlusRuntime {
public:
    GdiPlusRuntime()
    {
        Gdiplus::GdiplusStartupInput startup_input;
        const Gdiplus::Status status = Gdiplus::GdiplusStartup(&token_, &startup_input, nullptr);
        ready_ = (status == Gdiplus::Ok);
        if(!ready_) {
            token_ = 0;
        }
    }

    GdiPlusRuntime(const GdiPlusRuntime&) = delete;
    GdiPlusRuntime& operator=(const GdiPlusRuntime&) = delete;

    ~GdiPlusRuntime()
    {
        if(ready_ && token_ != 0) {
            Gdiplus::GdiplusShutdown(token_);
            token_ = 0;
        }
        ready_ = false;
    }

    [[nodiscard]] bool ready() const noexcept
    {
        return ready_;
    }

private:
    ULONG_PTR token_ = 0;
    bool ready_ = false;
};

[[nodiscard]] GdiPlusRuntime& gdiplus_runtime()
{
    static GdiPlusRuntime runtime;
    return runtime;
}

[[nodiscard]] std::wstring utf8_to_wide(std::string_view text)
{
    if(text.empty()) {
        return {};
    }

    if(text.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return {};
    }

    const int input_len = static_cast<int>(text.size());
    int required = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        text.data(),
        input_len,
        nullptr,
        0
    );
    unsigned int code_page = CP_UTF8;
    unsigned long flags = MB_ERR_INVALID_CHARS;

    if(required <= 0) {
        required = MultiByteToWideChar(CP_ACP, 0, text.data(), input_len, nullptr, 0);
        code_page = CP_ACP;
        flags = 0;
    }

    if(required <= 0) {
        return {};
    }

    std::wstring out(static_cast<std::size_t>(required), L'\0');
    const int converted = MultiByteToWideChar(
        code_page,
        flags,
        text.data(),
        input_len,
        out.data(),
        required
    );
    if(converted <= 0) {
        return {};
    }

    return out;
}
#endif

[[nodiscard]] bool load_image_rgba(
    std::string_view path,
    int& out_width,
    int& out_height,
    std::vector<unsigned char>& out_pixels
)
{
    out_width = 0;
    out_height = 0;
    out_pixels.clear();

#ifdef _WIN32
    if(path.empty()) {
        return false;
    }

    if(!gdiplus_runtime().ready()) {
        return false;
    }

    const std::wstring wide_path = utf8_to_wide(path);
    if(wide_path.empty()) {
        return false;
    }

    Gdiplus::Bitmap source_bitmap(wide_path.c_str());
    if(source_bitmap.GetLastStatus() != Gdiplus::Ok) {
        return false;
    }

    const unsigned int width = source_bitmap.GetWidth();
    const unsigned int height = source_bitmap.GetHeight();
    if(width == 0U || height == 0U) {
        return false;
    }

    if(width > static_cast<unsigned int>(std::numeric_limits<int>::max())
        || height > static_cast<unsigned int>(std::numeric_limits<int>::max())) {
        return false;
    }

    Gdiplus::Bitmap normalized_bitmap(
        static_cast<int>(width),
        static_cast<int>(height),
        PixelFormat32bppARGB
    );
    if(normalized_bitmap.GetLastStatus() != Gdiplus::Ok) {
        return false;
    }

    {
        Gdiplus::Graphics graphics(&normalized_bitmap);
        if(graphics.DrawImage(&source_bitmap, 0, 0, static_cast<int>(width), static_cast<int>(height))
            != Gdiplus::Ok) {
            return false;
        }
    }

    const std::size_t pixel_count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    out_pixels.assign(pixel_count * 4U, 0u);
    for(unsigned int y = 0U; y < height; ++y) {
        for(unsigned int x = 0U; x < width; ++x) {
            Gdiplus::Color pixel;
            if(normalized_bitmap.GetPixel(x, y, &pixel) != Gdiplus::Ok) {
                out_pixels.clear();
                return false;
            }

            const std::size_t dst = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width)
                + static_cast<std::size_t>(x))
                * 4U;
            out_pixels[dst + 0U] = pixel.GetRed();
            out_pixels[dst + 1U] = pixel.GetGreen();
            out_pixels[dst + 2U] = pixel.GetBlue();
            out_pixels[dst + 3U] = pixel.GetAlpha();
        }
    }

    out_width = static_cast<int>(width);
    out_height = static_cast<int>(height);
    return true;
#else
    (void)path;
    return false;
#endif
}

} // namespace

struct Glyph {
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
    int w = 0;
    int h = 0;
    int bearing_x = 0;
    int bearing_y = 0;
    int advance = 0;
};

struct FontKey {
    std::string path;
    int pixel_height = 0;

    [[nodiscard]] bool operator==(const FontKey& other) const noexcept = default;
};

struct FontKeyHasher {
    [[nodiscard]] std::size_t operator()(const FontKey& key) const noexcept
    {
        const std::size_t path_hash = std::hash<std::string>{}(key.path);
        const std::size_t height_hash = std::hash<int>{}(key.pixel_height);
        return path_hash ^ (height_hash + 0x9e3779b9u + (path_hash << 6u) + (path_hash >> 2u));
    }
};

struct FontResource {
    FT_Face primary_face = nullptr;
    FT_Face fallback_face = nullptr;

    GLuint glyph_texture = 0u;
    int glyph_tex_w = 0;
    int glyph_tex_h = 0;

    std::unordered_map<std::uint32_t, Glyph> glyphs;
    int atlas_pen_x = 1;
    int atlas_pen_y = 1;
    int atlas_row_h = 0;
    std::vector<unsigned char> atlas_pixels;

    std::size_t last_used_frame = 0U;
};

struct ImageResource {
    GLuint texture = 0u;
    int width = 0;
    int height = 0;
    std::size_t last_used_frame = 0U;
    bool load_failed = false;
};

class OpenGLRenderer final : public Renderer {
public:
    explicit OpenGLRenderer(platform::GlfwWindow& window)
        : window_(window)
    {
    }

    ~OpenGLRenderer() override
    {
        for(auto& kv : font_cache_) {
            destroy_font_resource(kv.second);
        }
        font_cache_.clear();

        for(auto& kv : image_cache_) {
            destroy_image_resource(kv.second);
        }
        image_cache_.clear();
    }

    void setup_2d_projection()
    {
        int w = 0;
        int h = 0;
        window_.get_framebuffer_size(w, h);
        if(w <= 0 || h <= 0) {
            return;
        }

        glViewport(0, 0, w, h);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, static_cast<double>(w), static_cast<double>(h), 0.0, -1.0, 1.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }

    void apply_scissor_rect(Rect rect)
    {
        int fb_w = 0;
        int fb_h = 0;
        window_.get_framebuffer_size(fb_w, fb_h);
        if(fb_w <= 0 || fb_h <= 0) {
            return;
        }

        if(is_empty(rect)) {
            glEnable(GL_SCISSOR_TEST);
            glScissor(0, 0, 0, 0);
            return;
        }

        const int x = static_cast<int>(rect.x);
        const int w = static_cast<int>(rect.w);

        const int y_bottom_left = fb_h - static_cast<int>(rect.y + rect.h);
        const int y = (y_bottom_left < 0) ? 0 : y_bottom_left;
        const int h = static_cast<int>(rect.h);

        glEnable(GL_SCISSOR_TEST);
        glScissor(x, y, w, h);
    }

    void clear(Color color) override
    {
        setup_2d_projection();
        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void fill_rect(Rect rect, Color color) override
    {
        setup_2d_projection();

        glColor4f(color.r, color.g, color.b, color.a);

        const float x0 = rect.x;
        const float y0 = rect.y;
        const float x1 = rect.x + rect.w;
        const float y1 = rect.y + rect.h;

        glBegin(GL_QUADS);
        glVertex2f(x0, y0);
        glVertex2f(x1, y0);
        glVertex2f(x1, y1);
        glVertex2f(x0, y1);
        glEnd();
    }

    void stroke_rect(Rect rect, Color color, float thickness) override
    {
        setup_2d_projection();

        glLineWidth(thickness);
        glColor4f(color.r, color.g, color.b, color.a);

        const float x0 = rect.x;
        const float y0 = rect.y;
        const float x1 = rect.x + rect.w;
        const float y1 = rect.y + rect.h;

        glBegin(GL_LINE_LOOP);
        glVertex2f(x0, y0);
        glVertex2f(x1, y0);
        glVertex2f(x1, y1);
        glVertex2f(x0, y1);
        glEnd();

        glLineWidth(1.0f);
    }

    void push_clip(Rect rect) override
    {
        if(!clip_stack_.empty()) {
            rect = intersect(clip_stack_.back(), rect);
        }

        clip_stack_.push_back(rect);
        apply_scissor_rect(clip_stack_.back());
    }

    void pop_clip() override
    {
        if(clip_stack_.empty()) {
            glDisable(GL_SCISSOR_TEST);
            return;
        }

        clip_stack_.pop_back();

        if(clip_stack_.empty()) {
            glDisable(GL_SCISSOR_TEST);
            return;
        }

        apply_scissor_rect(clip_stack_.back());
    }

    void present() override
    {
        window_.swap_buffers();
        ++frame_index_;
        collect_unused_resources();
    }

    bool set_font(std::string_view ttf_path, int pixel_height) override
    {
        if(ttf_path.empty() || pixel_height <= 0) {
            return false;
        }

        const std::string resolved_font_path = resolve_guinevere_path(ttf_path);
        if(resolved_font_path.empty()) {
            return false;
        }

        const FontKey key{resolved_font_path, pixel_height};
        FontResource* resource = ensure_font_resource(key);
        if(resource == nullptr) {
            return false;
        }

        resource->last_used_frame = frame_index_;
        selected_font_ = key;
        return true;
    }

    void draw_text(float x, float y, std::string_view text, Color color) override
    {
        FontResource* font = resolve_active_font();
        if(font == nullptr || font->glyph_texture == 0u || font->primary_face == nullptr) {
            return;
        }
        font->last_used_frame = frame_index_;

        setup_2d_projection();

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, font->glyph_texture);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(color.r, color.g, color.b, color.a);
        const NormalizedUtf8Text normalized_text = normalize_text_utf8(text);
        const std::string_view text_utf8 = normalized_text.view;

        // OpenGL immediate mode does not allow texture uploads between glBegin/glEnd.
        // Preload all required glyphs first so ensure_glyph can safely call glTexSubImage2D.
        std::vector<std::uint32_t> glyph_sequence;
        glyph_sequence.reserve(text_utf8.size());
        for(std::size_t i = 0; i < text_utf8.size();) {
            const Utf8DecodeResult dec = decode_one_utf8(text_utf8, i);
            if(dec.bytes_consumed == 0u) {
                break;
            }
            i += dec.bytes_consumed;

            std::uint32_t codepoint = dec.codepoint;
            const Glyph* g_ptr = ensure_glyph(*font, codepoint);
            if(g_ptr == nullptr) {
                codepoint = static_cast<std::uint32_t>('?');
                g_ptr = ensure_glyph(*font, codepoint);
            }
            if(g_ptr == nullptr) {
                codepoint = static_cast<std::uint32_t>(' ');
                g_ptr = ensure_glyph(*font, codepoint);
            }
            if(g_ptr != nullptr) {
                glyph_sequence.push_back(codepoint);
            }
        }

        float pen_x = x;
        const float pen_y = y;

        glBegin(GL_QUADS);
        for(const std::uint32_t codepoint : glyph_sequence) {
            const auto it = font->glyphs.find(codepoint);
            if(it == font->glyphs.end()) {
                continue;
            }

            const Glyph& g = it->second;
            if(g.w == 0 || g.h == 0) {
                pen_x += static_cast<float>(g.advance) / 64.0f;
                continue;
            }

            const float xpos = pen_x + static_cast<float>(g.bearing_x);
            const float ypos = pen_y - static_cast<float>(g.bearing_y);

            const float w = static_cast<float>(g.w);
            const float h = static_cast<float>(g.h);

            glTexCoord2f(g.u0, g.v0);
            glVertex2f(xpos, ypos);

            glTexCoord2f(g.u1, g.v0);
            glVertex2f(xpos + w, ypos);

            glTexCoord2f(g.u1, g.v1);
            glVertex2f(xpos + w, ypos + h);

            glTexCoord2f(g.u0, g.v1);
            glVertex2f(xpos, ypos + h);

            pen_x += static_cast<float>(g.advance) / 64.0f;
        }
        glEnd();

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
    }

    bool draw_image(Rect rect, std::string_view image_path, Color tint) override
    {
        const std::string resolved_image_path = resolve_guinevere_path(image_path);
        if(is_empty(rect) || resolved_image_path.empty()) {
            return false;
        }

        ImageResource* image = ensure_image_resource(resolved_image_path);
        if(image == nullptr || image->texture == 0u || image->load_failed) {
            return false;
        }
        image->last_used_frame = frame_index_;

        setup_2d_projection();

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, image->texture);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(tint.r, tint.g, tint.b, tint.a);

        const float x0 = rect.x;
        const float y0 = rect.y;
        const float x1 = rect.x + rect.w;
        const float y1 = rect.y + rect.h;

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(x0, y0);

        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(x1, y0);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(x1, y1);

        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(x0, y1);
        glEnd();

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        return true;
    }

    float measure_text(std::string_view text) override
    {
        FontResource* font = resolve_active_font();
        if(font == nullptr || font->glyph_texture == 0u || font->primary_face == nullptr) {
            return 0.0f;
        }
        font->last_used_frame = frame_index_;

        const NormalizedUtf8Text normalized_text = normalize_text_utf8(text);
        const std::string_view text_utf8 = normalized_text.view;
        float width = 0.0f;
        for(std::size_t i = 0; i < text_utf8.size();) {
            const Utf8DecodeResult dec = decode_one_utf8(text_utf8, i);
            if(dec.bytes_consumed == 0u) {
                break;
            }
            i += dec.bytes_consumed;

            std::uint32_t codepoint = dec.codepoint;
            const Glyph* g_ptr = ensure_glyph(*font, codepoint);
            if(g_ptr == nullptr) {
                codepoint = static_cast<std::uint32_t>('?');
                g_ptr = ensure_glyph(*font, codepoint);
            }
            if(g_ptr == nullptr) {
                codepoint = static_cast<std::uint32_t>(' ');
                g_ptr = ensure_glyph(*font, codepoint);
            }
            if(g_ptr != nullptr) {
                width += static_cast<float>(g_ptr->advance) / 64.0f;
            }
        }

        return width;
    }

private:
    static constexpr std::size_t kUnusedFrameBudget = 2U;

    platform::GlfwWindow& window_;
    std::vector<Rect> clip_stack_;

    guinevere::text::FreeTypeLibrary freetype_;
    std::unordered_map<FontKey, FontResource, FontKeyHasher> font_cache_;
    std::unordered_map<std::string, ImageResource> image_cache_;
    std::optional<FontKey> selected_font_;
    std::size_t frame_index_ = 0U;

    bool load_face(std::string_view path, int pixel_height, FT_Face& out_face)
    {
        if(path.empty() || pixel_height <= 0) {
            return false;
        }

        if(FT_New_Face(freetype_.get(), std::string(path).c_str(), 0, &out_face) != 0) {
            out_face = nullptr;
            return false;
        }

        if(!select_unicode_charmap(out_face)) {
            FT_Done_Face(out_face);
            out_face = nullptr;
            return false;
        }

        if(FT_Set_Pixel_Sizes(out_face, 0, static_cast<FT_UInt>(pixel_height)) != 0) {
            FT_Done_Face(out_face);
            out_face = nullptr;
            return false;
        }

        return true;
    }

    bool load_fallback_face(FontResource& resource, const char* ttf_path, int pixel_height)
    {
        if(resource.fallback_face != nullptr) {
            return true;
        }
        return load_face(ttf_path, pixel_height, resource.fallback_face);
    }

    void prewarm_common_glyphs(FontResource& resource)
    {
        // Prewarm frequently-changing UI glyphs to avoid first-interaction
        // stutter/flicker when counters and labels start changing.
        static constexpr std::uint32_t kCodepoints[] = {
            static_cast<std::uint32_t>(' '),
            static_cast<std::uint32_t>(':'),
            static_cast<std::uint32_t>('-'),
            static_cast<std::uint32_t>('.'),
            static_cast<std::uint32_t>('0'),
            static_cast<std::uint32_t>('1'),
            static_cast<std::uint32_t>('2'),
            static_cast<std::uint32_t>('3'),
            static_cast<std::uint32_t>('4'),
            static_cast<std::uint32_t>('5'),
            static_cast<std::uint32_t>('6'),
            static_cast<std::uint32_t>('7'),
            static_cast<std::uint32_t>('8'),
            static_cast<std::uint32_t>('9'),
        };

        for(const std::uint32_t codepoint : kCodepoints) {
            (void)ensure_glyph(resource, codepoint);
        }
    }

    void destroy_font_resource(FontResource& resource)
    {
        if(resource.primary_face != nullptr) {
            FT_Done_Face(resource.primary_face);
            resource.primary_face = nullptr;
        }

        if(resource.fallback_face != nullptr) {
            FT_Done_Face(resource.fallback_face);
            resource.fallback_face = nullptr;
        }

        if(resource.glyph_texture != 0u) {
            glDeleteTextures(1, &resource.glyph_texture);
            resource.glyph_texture = 0u;
        }

        resource.glyphs.clear();
        resource.atlas_pixels.clear();
        resource.glyph_tex_w = 0;
        resource.glyph_tex_h = 0;
        resource.atlas_pen_x = 1;
        resource.atlas_pen_y = 1;
        resource.atlas_row_h = 0;
    }

    void destroy_image_resource(ImageResource& resource)
    {
        if(resource.texture != 0u) {
            glDeleteTextures(1, &resource.texture);
            resource.texture = 0u;
        }
        resource.width = 0;
        resource.height = 0;
        resource.load_failed = false;
    }

    void collect_unused_resources()
    {
        for(auto it = font_cache_.begin(); it != font_cache_.end();) {
            const FontResource& resource = it->second;
            const bool stale = frame_index_ > resource.last_used_frame
                && (frame_index_ - resource.last_used_frame) > kUnusedFrameBudget;
            if(stale) {
                destroy_font_resource(it->second);
                it = font_cache_.erase(it);
                continue;
            }
            ++it;
        }

        for(auto it = image_cache_.begin(); it != image_cache_.end();) {
            const ImageResource& resource = it->second;
            const bool stale = frame_index_ > resource.last_used_frame
                && (frame_index_ - resource.last_used_frame) > kUnusedFrameBudget;
            if(stale) {
                destroy_image_resource(it->second);
                it = image_cache_.erase(it);
                continue;
            }
            ++it;
        }
    }

    bool ensure_default_font_loaded()
    {
        if(selected_font_.has_value()) {
            FontResource* selected = ensure_font_resource(*selected_font_);
            if(selected != nullptr) {
                selected->last_used_frame = frame_index_;
                return true;
            }
        }

#ifdef _WIN32
        const char* const candidates[] = {
            "Guinevere:/fonts/Google/Roboto-Regular.ttf",
            "C:/Windows/Fonts/segoeui.ttf",
            "C:/Windows/Fonts/seguisym.ttf",
            "C:/Windows/Fonts/arial.ttf"
        };
#else
        const char* const candidates[] = {
            "Guinevere:/fonts/Google/Roboto-Regular.ttf"
        };
#endif
        for(const char* path : candidates) {
            if(set_font(path, 24)) {
                return true;
            }
        }

        return false;
    }

    FontResource* ensure_font_resource(const FontKey& key)
    {
        const auto found = font_cache_.find(key);
        if(found != font_cache_.end()) {
            return &found->second;
        }

        FontResource fresh_resource;
        if(!load_face(key.path, key.pixel_height, fresh_resource.primary_face)) {
            destroy_font_resource(fresh_resource);
            return nullptr;
        }

#ifdef _WIN32
        const char* const fallback_candidates[] = {
            "C:/Windows/Fonts/segoeui.ttf",
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/tahoma.ttf",
            "C:/Windows/Fonts/arialuni.ttf"
        };
        for(const char* fallback_path : fallback_candidates) {
            if(load_fallback_face(fresh_resource, fallback_path, key.pixel_height)) {
                break;
            }
        }
#endif

        // Keep loading mostly lazy, but prewarm a tiny common set to avoid
        // visible first-interaction hitching on dynamic labels/counters.
        reset_glyph_atlas(fresh_resource);

        if(fresh_resource.glyph_texture == 0u) {
            destroy_font_resource(fresh_resource);
            return nullptr;
        }

        prewarm_common_glyphs(fresh_resource);

        fresh_resource.last_used_frame = frame_index_;
        auto [it, inserted] = font_cache_.emplace(key, std::move(fresh_resource));
        if(!inserted) {
            return &it->second;
        }
        return &it->second;
    }

    FontResource* resolve_active_font()
    {
        if(selected_font_.has_value()) {
            FontResource* selected = ensure_font_resource(*selected_font_);
            if(selected != nullptr) {
                return selected;
            }
        }

        if(!ensure_default_font_loaded()) {
            return nullptr;
        }

        if(!selected_font_.has_value()) {
            return nullptr;
        }

        return ensure_font_resource(*selected_font_);
    }

    bool upload_image_texture(ImageResource& resource, std::string_view path)
    {
        int image_w = 0;
        int image_h = 0;
        std::vector<unsigned char> pixels;
        if(!load_image_rgba(path, image_w, image_h, pixels)) {
            return false;
        }

        if(image_w <= 0 || image_h <= 0 || pixels.empty()) {
            return false;
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glGenTextures(1, &resource.texture);
        if(resource.texture == 0u) {
            return false;
        }

        glBindTexture(GL_TEXTURE_2D, resource.texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            image_w,
            image_h,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            pixels.data()
        );
        glBindTexture(GL_TEXTURE_2D, 0);

        resource.width = image_w;
        resource.height = image_h;
        resource.load_failed = false;
        return true;
    }

    ImageResource* ensure_image_resource(std::string_view image_path)
    {
        if(image_path.empty()) {
            return nullptr;
        }

        const std::string key(image_path);
        const auto found = image_cache_.find(key);
        if(found != image_cache_.end()) {
            if(found->second.texture == 0u || found->second.load_failed) {
                return nullptr;
            }
            return &found->second;
        }

        ImageResource fresh_resource;
        if(!upload_image_texture(fresh_resource, image_path)) {
            fresh_resource.load_failed = true;
            fresh_resource.last_used_frame = frame_index_;
            image_cache_.emplace(key, std::move(fresh_resource));
            return nullptr;
        }

        fresh_resource.last_used_frame = frame_index_;
        auto [it, inserted] = image_cache_.emplace(key, std::move(fresh_resource));
        if(!inserted) {
            return &it->second;
        }
        return &it->second;
    }

    void reset_glyph_atlas(FontResource& resource)
    {
        if(resource.glyph_texture != 0u) {
            glDeleteTextures(1, &resource.glyph_texture);
            resource.glyph_texture = 0u;
        }

        resource.glyphs.clear();

        resource.glyph_tex_w = 1024;
        resource.glyph_tex_h = 1024;

        resource.atlas_pen_x = 1;
        resource.atlas_pen_y = 1;
        resource.atlas_row_h = 0;

        resource.atlas_pixels.assign(
            static_cast<std::size_t>(resource.glyph_tex_w * resource.glyph_tex_h),
            0u
        );

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glGenTextures(1, &resource.glyph_texture);
        glBindTexture(GL_TEXTURE_2D, resource.glyph_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_ALPHA,
            resource.glyph_tex_w,
            resource.glyph_tex_h,
            0,
            GL_ALPHA,
            GL_UNSIGNED_BYTE,
            resource.atlas_pixels.data()
        );
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    const Glyph* ensure_glyph(FontResource& resource, std::uint32_t codepoint)
    {
        const auto it = resource.glyphs.find(codepoint);
        if(it != resource.glyphs.end()) {
            return &it->second;
        }

        if(resource.glyph_texture == 0u || resource.primary_face == nullptr) {
            return nullptr;
        }

        FT_Face selected_face = resource.primary_face;
        FT_UInt glyph_index = FT_Get_Char_Index(resource.primary_face, static_cast<FT_ULong>(codepoint));
        
        if(glyph_index == 0u) {
            if(resource.fallback_face != nullptr) {
                glyph_index = FT_Get_Char_Index(resource.fallback_face, static_cast<FT_ULong>(codepoint));
                if(glyph_index != 0u) {
                    selected_face = resource.fallback_face;
                } else {
                    return nullptr;
                }
            } else {
                return nullptr;
            }
        }

        if(FT_Load_Glyph(selected_face, glyph_index, FT_LOAD_DEFAULT) != 0) {
            return nullptr;
        }

        if(FT_Render_Glyph(selected_face->glyph, FT_RENDER_MODE_NORMAL) != 0) {
            return nullptr;
        }

        const FT_GlyphSlot g = selected_face->glyph;
        const int gw = static_cast<int>(g->bitmap.width);
        const int gh = static_cast<int>(g->bitmap.rows);
        const int pitch = static_cast<int>(g->bitmap.pitch);
        const int abs_pitch = (pitch < 0) ? -pitch : pitch;

        Glyph info;
        info.w = gw;
        info.h = gh;
        info.bearing_x = static_cast<int>(g->bitmap_left);
        info.bearing_y = static_cast<int>(g->bitmap_top);
        info.advance = static_cast<int>(g->advance.x);

        if(gw <= 0 || gh <= 0) {
            auto inserted = resource.glyphs.emplace(codepoint, info);
            return &inserted.first->second;
        }

        if(resource.atlas_pen_x + gw + 1 >= resource.glyph_tex_w) {
            resource.atlas_pen_x = 1;
            resource.atlas_pen_y += resource.atlas_row_h + 1;
            resource.atlas_row_h = 0;
        }

        if(resource.atlas_pen_y + gh + 1 >= resource.glyph_tex_h) {
            return nullptr;
        }

        std::vector<unsigned char> glyph_pixels(static_cast<std::size_t>(gw * gh), 0u);
        if(g->bitmap.buffer != nullptr) {
            for(int yy = 0; yy < gh; ++yy) {
                const int src_row = (pitch >= 0) ? yy : (gh - 1 - yy);
                for(int xx = 0; xx < gw; ++xx) {
                    unsigned char px = 0u;
                    switch(g->bitmap.pixel_mode) {
                    case FT_PIXEL_MODE_GRAY: {
                        const int src = src_row * abs_pitch + xx;
                        px = g->bitmap.buffer[src];
                        break;
                    }
                    case FT_PIXEL_MODE_MONO: {
                        const int src = src_row * abs_pitch + (xx >> 3);
                        const unsigned char bits = g->bitmap.buffer[src];
                        const unsigned char mask = static_cast<unsigned char>(0x80u >> (xx & 7));
                        px = ((bits & mask) != 0u) ? 255u : 0u;
                        break;
                    }
                    case FT_PIXEL_MODE_LCD: {
                        const int src = src_row * abs_pitch + (xx * 3);
                        const auto r = static_cast<unsigned int>(g->bitmap.buffer[src + 0]);
                        const auto gc = static_cast<unsigned int>(g->bitmap.buffer[src + 1]);
                        const auto b = static_cast<unsigned int>(g->bitmap.buffer[src + 2]);
                        px = static_cast<unsigned char>((r + gc + b) / 3u);
                        break;
                    }
                    case FT_PIXEL_MODE_LCD_V: {
                        // For vertical subpixel data, a simple approximation is to sample one channel.
                        const int src = src_row * abs_pitch + xx;
                        px = g->bitmap.buffer[src];
                        break;
                    }
                    case FT_PIXEL_MODE_BGRA: {
                        const int src = src_row * abs_pitch + (xx * 4) + 3;
                        px = g->bitmap.buffer[src];
                        break;
                    }
                    default:
                        px = 0u;
                        break;
                    }
                    glyph_pixels[static_cast<std::size_t>(yy * gw + xx)] = px;

                    const int dst = (resource.atlas_pen_y + yy) * resource.glyph_tex_w
                        + (resource.atlas_pen_x + xx);
                    resource.atlas_pixels[static_cast<std::size_t>(dst)] = px;
                }
            }
        }

        info.u0 = static_cast<float>(resource.atlas_pen_x) / static_cast<float>(resource.glyph_tex_w);
        info.v0 = static_cast<float>(resource.atlas_pen_y) / static_cast<float>(resource.glyph_tex_h);
        info.u1 = static_cast<float>(resource.atlas_pen_x + gw) / static_cast<float>(resource.glyph_tex_w);
        info.v1 = static_cast<float>(resource.atlas_pen_y + gh) / static_cast<float>(resource.glyph_tex_h);

        glBindTexture(GL_TEXTURE_2D, resource.glyph_texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            resource.atlas_pen_x,
            resource.atlas_pen_y,
            gw,
            gh,
            GL_ALPHA,
            GL_UNSIGNED_BYTE,
            glyph_pixels.data()
        );
        glBindTexture(GL_TEXTURE_2D, 0);

        resource.atlas_pen_x += gw + 1;
        if(gh > resource.atlas_row_h) {
            resource.atlas_row_h = gh;
        }

        auto inserted = resource.glyphs.emplace(codepoint, info);
        return &inserted.first->second;
    }
};

std::unique_ptr<Renderer> create_opengl_renderer(platform::GlfwWindow& window)
{
    return std::make_unique<OpenGLRenderer>(window);
}

} // namespace guinevere::gfx
