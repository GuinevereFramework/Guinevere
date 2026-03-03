#include <iostream>

#include <guinevere/style/style_engine.hpp>

int main()
{
    using namespace guinevere::style;

    StyleSheet sheet;

    {
        Rule r;
        r.selector.type = "Button";
        r.declarations.push_back({"padding", "8"});
        r.declarations.push_back({"background", "#333333"});
        sheet.rules.push_back(std::move(r));
    }

    {
        Rule r;
        r.selector.type = "Button";
        r.selector.classes.push_back("primary");
        r.declarations.push_back({"background", "#0066ff"});
        sheet.rules.push_back(std::move(r));
    }

    {
        Rule r;
        r.selector.id = "ok";
        r.declarations.push_back({"background", "#00aa00"});
        sheet.rules.push_back(std::move(r));
    }

    {
        Rule r;
        r.selector.type = "Button";
        r.selector.required_state = PseudoClass::Hover;
        r.declarations.push_back({"background", "#777777"});
        sheet.rules.push_back(std::move(r));
    }

    StyleEngine engine;
    engine.set_stylesheet(std::move(sheet));

    NodeRef button;
    button.type = "Button";
    button.id = "ok";
    button.classes = {"primary"};
    button.state = PseudoClass::Hover;

    const ComputedStyle style = engine.compute(button);

    std::cout << "Computed style for Button#ok.primary:hover\n";

    for(const auto& kv : style.properties) {
        std::cout << kv.first << ": " << kv.second << "\n";
    }

    return 0;
}
