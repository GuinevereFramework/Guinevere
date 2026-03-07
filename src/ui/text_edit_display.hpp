#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include <guinevere/ui/types.hpp>

namespace guinevere::ui::detail {

[[nodiscard]] inline bool is_utf8_continuation_byte(unsigned char byte) noexcept
{
    return (byte & 0xC0u) == 0x80u;
}

[[nodiscard]] inline std::size_t next_utf8_boundary(std::string_view text, std::size_t cursor) noexcept
{
    std::size_t i = std::min(cursor, text.size());
    if(i >= text.size()) {
        return text.size();
    }

    ++i;
    while(i < text.size() && is_utf8_continuation_byte(static_cast<unsigned char>(text[i]))) {
        ++i;
    }
    return i;
}

[[nodiscard]] inline bool is_password_input_type(TextEditInputType input_type) noexcept
{
    return input_type == TextEditInputType::PasswordLine;
}

[[nodiscard]] inline bool is_multiline_input_type(TextEditInputType input_type) noexcept
{
    return input_type == TextEditInputType::MultiLine;
}

[[nodiscard]] inline TextEditEchoMode effective_echo_mode(
    TextEditInputType input_type,
    TextEditEchoMode echo_mode
) noexcept
{
    return is_password_input_type(input_type) ? echo_mode : TextEditEchoMode::normal();
}

struct TextEditDisplayText {
    std::string text;
    std::vector<std::size_t> source_boundaries;
    std::vector<std::size_t> display_boundaries;
};

[[nodiscard]] inline bool should_reveal_codepoint(
    TextEditEchoMode echo_mode,
    std::size_t source_begin,
    std::size_t source_end,
    std::size_t reveal_begin,
    std::size_t reveal_end,
    double reveal_until_seconds,
    double elapsed_seconds
) noexcept
{
    if(echo_mode.kind != TextEditEchoModeKind::ShowIn) {
        return false;
    }
    if(elapsed_seconds >= reveal_until_seconds) {
        return false;
    }
    return source_begin >= reveal_begin && source_end <= reveal_end;
}

[[nodiscard]] inline TextEditDisplayText build_text_edit_display_text(
    std::string_view source,
    TextEditInputType input_type,
    TextEditEchoMode echo_mode,
    std::size_t reveal_begin = 0U,
    std::size_t reveal_end = 0U,
    double reveal_until_seconds = 0.0,
    double elapsed_seconds = 0.0
)
{
    TextEditDisplayText display;
    display.source_boundaries.reserve(source.size() + 1U);
    display.display_boundaries.reserve(source.size() + 1U);
    display.source_boundaries.push_back(0U);
    display.display_boundaries.push_back(0U);

    const TextEditEchoMode effective_mode = effective_echo_mode(input_type, echo_mode);
    if(effective_mode.kind == TextEditEchoModeKind::Normal) {
        display.text.assign(source);
        for(std::size_t i = 0U; i < source.size();) {
            i = next_utf8_boundary(source, i);
            display.source_boundaries.push_back(i);
            display.display_boundaries.push_back(i);
        }
        return display;
    }

    for(std::size_t source_begin = 0U; source_begin < source.size();) {
        const std::size_t source_end = next_utf8_boundary(source, source_begin);
        const std::string_view codepoint = source.substr(source_begin, source_end - source_begin);

        if(effective_mode.kind == TextEditEchoModeKind::NoEcho) {
        } else if(should_reveal_codepoint(
                      effective_mode,
                      source_begin,
                      source_end,
                      reveal_begin,
                      reveal_end,
                      reveal_until_seconds,
                      elapsed_seconds
                  )) {
            display.text.append(codepoint);
        } else {
            display.text.push_back('*');
        }

        display.source_boundaries.push_back(source_end);
        display.display_boundaries.push_back(display.text.size());
        source_begin = source_end;
    }

    return display;
}

[[nodiscard]] inline std::size_t display_index_from_source(
    const TextEditDisplayText& display,
    std::size_t source_index
) noexcept
{
    for(std::size_t i = 0U; i < display.source_boundaries.size(); ++i) {
        if(display.source_boundaries[i] == source_index) {
            return display.display_boundaries[i];
        }
    }
    return display.display_boundaries.empty() ? 0U : display.display_boundaries.back();
}

[[nodiscard]] inline std::size_t source_index_from_display(
    const TextEditDisplayText& display,
    std::size_t display_index
) noexcept
{
    for(std::size_t i = 0U; i < display.display_boundaries.size(); ++i) {
        if(display_index <= display.display_boundaries[i]) {
            return display.source_boundaries[i];
        }
    }
    return display.source_boundaries.empty() ? 0U : display.source_boundaries.back();
}

} // namespace guinevere::ui::detail
