#pragma once

#include <guinevere/ui/interaction.hpp>
#include <guinevere/ui/state_store.hpp>

namespace guinevere::ui {

class UiRuntime {
public:
    class FrameScope {
    public:
        FrameScope(UiRuntime& runtime, app::Context& context, std::optional<gfx::Color> clear_color) noexcept;
        FrameScope(const FrameScope&) = delete;
        FrameScope& operator=(const FrameScope&) = delete;
        FrameScope(FrameScope&& other) noexcept;
        FrameScope& operator=(FrameScope&& other) noexcept;
        ~FrameScope();

        [[nodiscard]] FrameBuilder& frame_builder() noexcept;
        [[nodiscard]] StateStore& state_store() noexcept;
        [[nodiscard]] ComponentScope root_component(std::string component_key_prefix);
        [[nodiscard]] app::Context& context() noexcept;
        [[nodiscard]] bool end();

    private:
        UiRuntime* runtime_ = nullptr;
        app::Context* context_ = nullptr;
        std::optional<gfx::Color> clear_color_ = std::nullopt;
        bool ended_ = false;
    };

    explicit UiRuntime(std::string root_key = "root");

    [[nodiscard]] std::string_view root_key() const noexcept
    {
        return root_key_;
    }

    [[nodiscard]] UiTree& tree() noexcept
    {
        return tree_;
    }

    [[nodiscard]] const UiTree& tree() const noexcept
    {
        return tree_;
    }

    [[nodiscard]] StateStore& state_store() noexcept
    {
        return state_store_;
    }

    [[nodiscard]] const StateStore& state_store() const noexcept
    {
        return state_store_;
    }

    [[nodiscard]] FrameBuilder& frame_builder() noexcept
    {
        return frame_builder_;
    }

    [[nodiscard]] const std::vector<ReconciledNode>& frame() const noexcept
    {
        return frame_;
    }

    [[nodiscard]] InteractionState& interactions() noexcept
    {
        return interactions_;
    }

    [[nodiscard]] const InteractionState& interactions() const noexcept
    {
        return interactions_;
    }

    [[nodiscard]] ComponentScope root_component(std::string component_key_prefix)
    {
        return ComponentScope(frame_builder_, state_store_, root_key_, std::move(component_key_prefix));
    }

    void reserve(std::size_t count)
    {
        frame_builder_.reserve(count);
    }

    [[nodiscard]] gfx::Rect viewport(app::Context& context) const noexcept;
    [[nodiscard]] AppScaffoldResult resolve_scaffold(
        app::Context& context,
        const AppScaffoldSpec& spec = AppScaffoldSpec{}
    ) noexcept;

    template<typename BuildFn>
    bool app_frame(
        app::Context& context,
        BuildFn&& build,
        std::optional<gfx::Color> clear_color = std::nullopt
    )
    {
        auto frame_scope = frame(context, clear_color);
        app::Context& frame_context = frame_scope.context();

        if constexpr(std::is_invocable_r_v<bool, BuildFn, app::Context&, UiRuntime&>) {
            const bool keep_running = std::invoke(
                std::forward<BuildFn>(build),
                frame_context,
                *this
            );
            (void)frame_scope.end();
            return keep_running;
        } else {
            static_assert(
                std::is_invocable_v<BuildFn, app::Context&, UiRuntime&>,
                "UiRuntime::app_frame callback must accept (app::Context&, UiRuntime&) and return void or bool."
            );
            std::invoke(std::forward<BuildFn>(build), frame_context, *this);
            (void)frame_scope.end();
            return true;
        }
    }

    template<typename BuildFn>
    bool component_frame(
        app::Context& context,
        std::string component_key_prefix,
        BuildFn&& build,
        std::optional<gfx::Color> clear_color = std::nullopt
    )
    {
        auto frame_scope = frame(context, clear_color);
        ComponentScope root = frame_scope.root_component(std::move(component_key_prefix));

        if constexpr(std::is_invocable_r_v<bool, BuildFn, ComponentScope&>) {
            const bool keep_running = std::invoke(
                std::forward<BuildFn>(build),
                root
            );
            (void)frame_scope.end();
            return keep_running;
        } else if constexpr(std::is_invocable_r_v<bool, BuildFn, ComponentScope&, UiRuntime&>) {
            const bool keep_running = std::invoke(
                std::forward<BuildFn>(build),
                root,
                *this
            );
            (void)frame_scope.end();
            return keep_running;
        } else if constexpr(std::is_invocable_v<BuildFn, ComponentScope&>) {
            std::invoke(std::forward<BuildFn>(build), root);
            (void)frame_scope.end();
            return true;
        } else {
            static_assert(
                std::is_invocable_v<BuildFn, ComponentScope&, UiRuntime&>,
                "UiRuntime::component_frame callback must accept (ComponentScope&) or (ComponentScope&, UiRuntime&) and return void or bool."
            );
            std::invoke(std::forward<BuildFn>(build), root, *this);
            (void)frame_scope.end();
            return true;
        }
    }

    template<typename BuildFn>
    bool component_frame(
        app::Context& context,
        BuildFn&& build,
        std::optional<gfx::Color> clear_color = std::nullopt
    )
    {
        return component_frame(
            context,
            std::string("app"),
            std::forward<BuildFn>(build),
            clear_color
        );
    }

    template<typename BuildFn>
    bool app_frame(
        app::Context& context,
        const AppScaffoldSpec& scaffold_spec,
        BuildFn&& build,
        std::optional<gfx::Color> clear_color = std::nullopt
    )
    {
        auto frame_scope = frame(context, clear_color);
        app::Context& frame_context = frame_scope.context();
        const AppScaffoldResult scaffold = resolve_scaffold(frame_context, scaffold_spec);

        if constexpr(std::is_invocable_r_v<
                         bool,
                         BuildFn,
                         app::Context&,
                         UiRuntime&,
                         const AppScaffoldResult&>) {
            const bool keep_running = std::invoke(
                std::forward<BuildFn>(build),
                frame_context,
                *this,
                scaffold
            );
            (void)frame_scope.end();
            return keep_running;
        } else {
            static_assert(
                std::is_invocable_v<
                    BuildFn,
                    app::Context&,
                    UiRuntime&,
                    const AppScaffoldResult&>,
                "UiRuntime::app_frame callback must accept (app::Context&, UiRuntime&, const AppScaffoldResult&) and return void or bool."
            );
            std::invoke(std::forward<BuildFn>(build), frame_context, *this, scaffold);
            (void)frame_scope.end();
            return true;
        }
    }

    template<typename BuildFn>
    bool component_frame(
        app::Context& context,
        const AppScaffoldSpec& scaffold_spec,
        std::string component_key_prefix,
        BuildFn&& build,
        std::optional<gfx::Color> clear_color = std::nullopt
    )
    {
        auto frame_scope = frame(context, clear_color);
        app::Context& frame_context = frame_scope.context();
        const AppScaffoldResult scaffold = resolve_scaffold(frame_context, scaffold_spec);
        ComponentScope root = frame_scope.root_component(std::move(component_key_prefix));

        if constexpr(std::is_invocable_r_v<
                         bool,
                         BuildFn,
                         ComponentScope&,
                         const AppScaffoldResult&>) {
            const bool keep_running = std::invoke(
                std::forward<BuildFn>(build),
                root,
                scaffold
            );
            (void)frame_scope.end();
            return keep_running;
        } else if constexpr(std::is_invocable_r_v<
                                bool,
                                BuildFn,
                                ComponentScope&,
                                UiRuntime&,
                                const AppScaffoldResult&>) {
            const bool keep_running = std::invoke(
                std::forward<BuildFn>(build),
                root,
                *this,
                scaffold
            );
            (void)frame_scope.end();
            return keep_running;
        } else if constexpr(std::is_invocable_v<
                                BuildFn,
                                ComponentScope&,
                                const AppScaffoldResult&>) {
            std::invoke(std::forward<BuildFn>(build), root, scaffold);
            (void)frame_scope.end();
            return true;
        } else {
            static_assert(
                std::is_invocable_v<
                    BuildFn,
                    ComponentScope&,
                    UiRuntime&,
                    const AppScaffoldResult&>,
                "UiRuntime::component_frame callback must accept (ComponentScope&, const AppScaffoldResult&) or (ComponentScope&, UiRuntime&, const AppScaffoldResult&) and return void or bool."
            );
            std::invoke(std::forward<BuildFn>(build), root, *this, scaffold);
            (void)frame_scope.end();
            return true;
        }
    }

    template<typename BuildFn>
    bool component_frame(
        app::Context& context,
        const AppScaffoldSpec& scaffold_spec,
        BuildFn&& build,
        std::optional<gfx::Color> clear_color = std::nullopt
    )
    {
        return component_frame(
            context,
            scaffold_spec,
            std::string("app"),
            std::forward<BuildFn>(build),
            clear_color
        );
    }

    void begin_frame(InputState input) noexcept;
    void begin_frame(app::Context& context) noexcept;
    [[nodiscard]] FrameScope frame(
        app::Context& context,
        std::optional<gfx::Color> clear_color = std::nullopt
    ) noexcept;

    bool end_frame(
        gfx::Renderer& renderer,
        gfx::Rect viewport,
        std::optional<gfx::Color> clear_color = std::nullopt
    );
    bool end_frame(
        gfx::Renderer& renderer,
        gfx::Rect viewport,
        const asset::AssetManager& assets,
        std::optional<gfx::Color> clear_color = std::nullopt
    );
    bool end_frame(
        app::Context& context,
        std::optional<gfx::Color> clear_color = std::nullopt
    );

private:
    bool end_frame(
        gfx::Renderer& renderer,
        gfx::Rect viewport,
        const asset::AssetManager* assets,
        std::optional<gfx::Color> clear_color
    );

    UiTree tree_{};
    Pipeline pipeline_{};
    InteractionState interactions_{};
    StateStore state_store_{};
    std::vector<ReconciledNode> frame_{};
    FrameBuilder frame_builder_;
    std::string root_key_;
};

} // namespace guinevere::ui
