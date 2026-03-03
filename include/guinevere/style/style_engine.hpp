#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace guinevere::style {

enum class PseudoClass : std::uint32_t {
    None = 0,
    Hover = 1u << 0,
    Active = 1u << 1,
    Disabled = 1u << 2,
    Focus = 1u << 3,
};

constexpr PseudoClass operator|(PseudoClass a, PseudoClass b)
{
    return static_cast<PseudoClass>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

constexpr bool has_flag(PseudoClass value, PseudoClass flag)
{
    return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(flag)) != 0;
}

struct NodeRef {
    std::string type;
    std::string id;
    std::vector<std::string> classes;
    PseudoClass state = PseudoClass::None;
};

struct Selector {
    std::optional<std::string> type;
    std::optional<std::string> id;
    std::vector<std::string> classes;
    PseudoClass required_state = PseudoClass::None;

    bool matches(const NodeRef& node) const;
    std::uint32_t specificity() const;
};

struct Declaration {
    std::string property;
    std::string value;
};

struct Rule {
    Selector selector;
    std::vector<Declaration> declarations;
};

struct StyleSheet {
    std::vector<Rule> rules;
};

struct ComputedStyle {
    std::unordered_map<std::string, std::string> properties;

    std::optional<std::string_view> get(std::string_view key) const;
};

class StyleEngine {
public:
    void set_stylesheet(StyleSheet sheet);
    ComputedStyle compute(const NodeRef& node) const;

private:
    StyleSheet sheet_;
};

} // namespace guinevere::style
