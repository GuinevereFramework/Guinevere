#include <algorithm>

#include <guinevere/style/style_engine.hpp>

namespace guinevere::style {

static bool contains_class(const std::vector<std::string>& classes, const std::string& value)
{
    return std::find(classes.begin(), classes.end(), value) != classes.end();
}

bool Selector::matches(const NodeRef& node) const
{
    if(type.has_value() && *type != node.type) {
        return false;
    }

    if(id.has_value() && *id != node.id) {
        return false;
    }

    for(const auto& cls : classes) {
        if(!contains_class(node.classes, cls)) {
            return false;
        }
    }

    if(required_state != PseudoClass::None) {
        if(!has_flag(node.state, required_state)) {
            return false;
        }
    }

    return true;
}

std::uint32_t Selector::specificity() const
{
    std::uint32_t score = 0;

    if(id.has_value()) {
        score += 100;
    }

    if(type.has_value()) {
        score += 10;
    }

    score += static_cast<std::uint32_t>(classes.size());

    if(required_state != PseudoClass::None) {
        score += 1;
    }

    return score;
}

std::optional<std::string_view> ComputedStyle::get(std::string_view key) const
{
    const auto it = properties.find(std::string(key));
    if(it == properties.end()) {
        return std::nullopt;
    }

    return std::string_view(it->second);
}

void StyleEngine::set_stylesheet(StyleSheet sheet)
{
    sheet_ = std::move(sheet);
}

ComputedStyle StyleEngine::compute(const NodeRef& node) const
{
    struct MatchedRule {
        const Rule* rule = nullptr;
        std::uint32_t specificity = 0;
        std::size_t order = 0;
    };

    std::vector<MatchedRule> matched;
    matched.reserve(sheet_.rules.size());

    for(std::size_t i = 0; i < sheet_.rules.size(); ++i) {
        const auto& rule = sheet_.rules[i];
        if(rule.selector.matches(node)) {
            matched.push_back(MatchedRule{&rule, rule.selector.specificity(), i});
        }
    }

    std::stable_sort(matched.begin(), matched.end(), [](const MatchedRule& a, const MatchedRule& b) {
        if(a.specificity != b.specificity) {
            return a.specificity < b.specificity;
        }

        return a.order < b.order;
    });

    ComputedStyle result;

    for(const auto& m : matched) {
        for(const auto& decl : m.rule->declarations) {
            result.properties[decl.property] = decl.value;
        }
    }

    return result;
}

} // namespace guinevere::style
