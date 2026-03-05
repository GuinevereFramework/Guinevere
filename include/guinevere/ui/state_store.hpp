#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <source_location>
#include <unordered_map>

#include <guinevere/ui/frame_builder.hpp>

namespace guinevere::ui {

namespace detail {

inline constexpr char k_scope_key_separator = '.';

[[nodiscard]] inline bool is_valid_key_segment(std::string_view key_segment) noexcept
{
    return !key_segment.empty()
        && key_segment.find(k_scope_key_separator) == std::string_view::npos;
}

[[nodiscard]] inline bool is_valid_qualified_key(std::string_view key) noexcept
{
    if(key.empty()) {
        return false;
    }

    bool previous_was_separator = true;
    for(const char c : key) {
        if(c == k_scope_key_separator) {
            if(previous_was_separator) {
                return false;
            }
            previous_was_separator = true;
            continue;
        }
        previous_was_separator = false;
    }

    return !previous_was_separator;
}

inline void validate_key_segment(std::string_view key_segment, std::string_view context_name)
{
    if(key_segment.empty()) {
        throw std::invalid_argument(std::string(context_name) + " must not be empty.");
    }
    if(key_segment.find(k_scope_key_separator) != std::string_view::npos) {
        throw std::invalid_argument(
            std::string(context_name) + " must not contain '" + k_scope_key_separator + "'."
        );
    }
}

inline void validate_qualified_key(std::string_view key, std::string_view context_name)
{
    if(!is_valid_qualified_key(key)) {
        throw std::invalid_argument(
            std::string(context_name)
            + " must be a non-empty dotted path with non-empty segments."
        );
    }
}

[[nodiscard]] inline std::uint64_t hash_append_u64(
    std::uint64_t seed,
    std::uint64_t value
) noexcept
{
    constexpr std::uint64_t prime = 1099511628211ull;
    std::uint64_t hash = seed;
    std::uint64_t input = value;
    for(std::size_t i = 0U; i < sizeof(std::uint64_t); ++i) {
        const std::uint8_t byte = static_cast<std::uint8_t>(input & 0xFFull);
        hash ^= static_cast<std::uint64_t>(byte);
        hash *= prime;
        input >>= 8U;
    }
    return hash;
}

[[nodiscard]] inline std::uint64_t fnv1a_hash_text(
    std::uint64_t seed,
    std::string_view text
) noexcept
{
    constexpr std::uint64_t prime = 1099511628211ull;
    std::uint64_t hash = seed;
    for(const char c : text) {
        hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
        hash *= prime;
    }
    return hash;
}

[[nodiscard]] inline std::string make_auto_key_segment(
    std::string_view prefix,
    std::size_t salt,
    const std::source_location& location
)
{
    validate_key_segment(prefix, "ComponentScope auto key prefix");

    std::uint64_t hash = 1469598103934665603ull;
    hash = fnv1a_hash_text(hash, location.file_name());
    hash = fnv1a_hash_text(hash, location.function_name());
    hash = hash_append_u64(hash, static_cast<std::uint64_t>(location.line()));
    hash = hash_append_u64(hash, static_cast<std::uint64_t>(location.column()));
    hash = hash_append_u64(hash, static_cast<std::uint64_t>(salt));

    constexpr char hex_digits[] = "0123456789abcdef";
    std::string key_segment;
    key_segment.reserve(prefix.size() + 1U + 16U);
    key_segment.append(prefix);
    key_segment.push_back('_');

    for(int shift = 60; shift >= 0; shift -= 4) {
        const std::uint64_t nibble = (hash >> static_cast<std::uint64_t>(shift)) & 0xFull;
        key_segment.push_back(hex_digits[static_cast<std::size_t>(nibble)]);
    }

    return key_segment;
}

[[nodiscard]] inline std::uint64_t hash_source_location(
    const std::source_location& location
) noexcept
{
    std::uint64_t hash = 1469598103934665603ull;
    hash = fnv1a_hash_text(hash, location.file_name());
    hash = fnv1a_hash_text(hash, location.function_name());
    hash = hash_append_u64(hash, static_cast<std::uint64_t>(location.line()));
    hash = hash_append_u64(hash, static_cast<std::uint64_t>(location.column()));
    return hash;
}

template<std::size_t N>
struct FixedString {
    char value[N]{};

    constexpr FixedString(const char (&text)[N])
    {
        for(std::size_t i = 0; i < N; ++i) {
            value[i] = text[i];
        }
    }

    [[nodiscard]] constexpr std::string_view view() const noexcept
    {
        return std::string_view(value, N - 1U);
    }
};

template<std::size_t N>
FixedString(const char (&)[N]) -> FixedString<N>;

[[nodiscard]] consteval bool is_valid_key_segment_literal(std::string_view key_segment)
{
    if(key_segment.empty()) {
        return false;
    }
    for(const char c : key_segment) {
        if(c == k_scope_key_separator) {
            return false;
        }
    }
    return true;
}

template<FixedString Name>
struct LocalKeyTag {
    static constexpr std::string_view value = Name.view();
    static_assert(
        is_valid_key_segment_literal(value),
        "LocalKey must be a non-empty key segment without '.'"
    );
};

template<typename ValueType, FixedString Name>
struct StateKeyTag {
    using value_type = ValueType;
    static constexpr std::string_view value = Name.view();
    static_assert(
        is_valid_key_segment_literal(value),
        "StateKey must be a non-empty key segment without '.'"
    );
};

template<typename Key>
concept LocalKeyType = requires {
    { Key::value } -> std::convertible_to<std::string_view>;
};

template<typename Key>
concept StateKeyType = requires {
    typename Key::value_type;
    { Key::value } -> std::convertible_to<std::string_view>;
};

} // namespace detail

template<detail::FixedString Name>
using LocalKey = detail::LocalKeyTag<Name>;

template<typename ValueType, detail::FixedString Name>
using StateKey = detail::StateKeyTag<ValueType, Name>;

class StateStore {
public:
    enum class ChangeKind {
        Set,
        Erase,
    };

    struct ChangeEvent {
        std::string_view key;
        ChangeKind kind = ChangeKind::Set;
    };

    using Observer = std::function<void(const ChangeEvent&)>;

    class Subscription {
    public:
        Subscription() = default;
        Subscription(const Subscription&) = delete;
        Subscription& operator=(const Subscription&) = delete;

        Subscription(Subscription&& other) noexcept
            : store_(other.store_)
            , observer_id_(other.observer_id_)
        {
            other.store_ = nullptr;
            other.observer_id_ = 0;
        }

        Subscription& operator=(Subscription&& other) noexcept
        {
            if(this == &other) {
                return *this;
            }

            reset();
            store_ = other.store_;
            observer_id_ = other.observer_id_;
            other.store_ = nullptr;
            other.observer_id_ = 0;
            return *this;
        }

        ~Subscription()
        {
            reset();
        }

        void reset() noexcept
        {
            if(store_ != nullptr && observer_id_ != 0) {
                store_->unsubscribe(observer_id_);
            }
            store_ = nullptr;
            observer_id_ = 0;
        }

        [[nodiscard]] bool active() const noexcept
        {
            return store_ != nullptr && observer_id_ != 0;
        }

    private:
        friend class StateStore;

        Subscription(StateStore* store, std::size_t observer_id) noexcept
            : store_(store)
            , observer_id_(observer_id)
        {
        }

        StateStore* store_ = nullptr;
        std::size_t observer_id_ = 0;
    };

    class Scope {
    public:
        Scope(StateStore* store, std::string prefix)
            : store_(store)
            , prefix_(std::move(prefix))
        {
            if(store_ == nullptr) {
                throw std::invalid_argument("StateStore::Scope store must not be null.");
            }
            if(!prefix_.empty()) {
                detail::validate_qualified_key(prefix_, "StateStore::Scope prefix");
            }
        }

        template<typename T>
        T& use(std::string_view key, T initial = T{})
        {
            return store_->use<T>(compose_key(key), std::move(initial));
        }

        template<detail::StateKeyType Key>
        typename Key::value_type& use(
            typename Key::value_type initial = typename Key::value_type{}
        )
        {
            return use<typename Key::value_type>(Key::value, std::move(initial));
        }

        template<typename T>
        T* find(std::string_view key)
        {
            return store_->find<T>(compose_key(key));
        }

        template<detail::StateKeyType Key>
        typename Key::value_type* find()
        {
            return find<typename Key::value_type>(Key::value);
        }

        template<typename T>
        const T* find(std::string_view key) const
        {
            return store_->find<T>(compose_key(key));
        }

        template<detail::StateKeyType Key>
        const typename Key::value_type* find() const
        {
            return find<typename Key::value_type>(Key::value);
        }

        template<typename T>
        void set(std::string_view key, T value)
        {
            store_->set<T>(compose_key(key), std::move(value));
        }

        template<detail::StateKeyType Key>
        void set(typename Key::value_type value)
        {
            set<typename Key::value_type>(Key::value, std::move(value));
        }

        template<typename T, typename Updater>
        T& update(std::string_view key, Updater&& updater)
        {
            return store_->update<T>(compose_key(key), std::forward<Updater>(updater));
        }

        template<detail::StateKeyType Key, typename Updater>
        typename Key::value_type& update(Updater&& updater)
        {
            return update<typename Key::value_type>(Key::value, std::forward<Updater>(updater));
        }

        [[nodiscard]] Subscription observe(std::string_view key, Observer observer)
        {
            return store_->observe(compose_key(key), std::move(observer));
        }

        [[nodiscard]] Subscription observe_prefix(std::string_view key_prefix, Observer observer)
        {
            return store_->observe_prefix(compose_key(key_prefix), std::move(observer));
        }

        [[nodiscard]] bool contains(std::string_view key) const
        {
            return store_->contains(compose_key(key));
        }

        template<detail::StateKeyType Key>
        [[nodiscard]] bool contains() const
        {
            return contains(Key::value);
        }

        void erase(std::string_view key)
        {
            store_->erase(compose_key(key));
        }

        template<detail::StateKeyType Key>
        void erase()
        {
            erase(Key::value);
        }

        void clear()
        {
            store_->erase_prefix(prefix_);
        }

        [[nodiscard]] std::string_view prefix() const noexcept
        {
            return prefix_;
        }

    private:
        [[nodiscard]] std::string compose_key(std::string_view key) const
        {
            if(!key.empty()) {
                detail::validate_key_segment(key, "StateStore::Scope key");
            }

            if(prefix_.empty()) {
                return std::string(key);
            }

            if(key.empty()) {
                return prefix_;
            }

            std::string full_key;
            full_key.reserve(prefix_.size() + key.size() + 1U);
            full_key.append(prefix_);
            full_key.push_back(detail::k_scope_key_separator);
            full_key.append(key);
            return full_key;
        }

        StateStore* store_ = nullptr;
        std::string prefix_;
    };

    template<typename T>
    T& use(std::string key, T initial = T{})
    {
        auto [it, inserted] = values_.try_emplace(std::move(key), std::move(initial));
        if(inserted) {
            T* value = std::any_cast<T>(&it->second);
            if(value == nullptr) {
                throw std::runtime_error("StateStore failed to initialize state value.");
            }
            notify_change(it->first, ChangeKind::Set);
            return *value;
        }

        T* value = std::any_cast<T>(&it->second);
        if(value == nullptr) {
            throw std::runtime_error("StateStore type mismatch for existing key.");
        }
        return *value;
    }

    template<typename T>
    T* find(std::string_view key)
    {
        const auto it = values_.find(std::string(key));
        if(it == values_.end()) {
            return nullptr;
        }
        return std::any_cast<T>(&it->second);
    }

    template<typename T>
    const T* find(std::string_view key) const
    {
        const auto it = values_.find(std::string(key));
        if(it == values_.end()) {
            return nullptr;
        }
        return std::any_cast<T>(&it->second);
    }

    template<typename T>
    void set(std::string key, T value)
    {
        auto [it, _] = values_.insert_or_assign(std::move(key), std::move(value));
        notify_change(it->first, ChangeKind::Set);
    }

    template<typename T, typename Updater>
    T& update(std::string key, Updater&& updater)
    {
        auto [it, inserted] = values_.try_emplace(std::move(key), T{});
        T* value = std::any_cast<T>(&it->second);
        if(value == nullptr) {
            throw std::runtime_error("StateStore type mismatch for existing key.");
        }

        std::invoke(std::forward<Updater>(updater), *value);
        if(inserted) {
            notify_change(it->first, ChangeKind::Set);
            return *value;
        }

        notify_change(it->first, ChangeKind::Set);
        return *value;
    }

    [[nodiscard]] bool contains(std::string_view key) const
    {
        return values_.find(std::string(key)) != values_.end();
    }

    [[nodiscard]] Subscription observe(std::string key, Observer observer);
    [[nodiscard]] Subscription observe_prefix(std::string prefix, Observer observer);

    void erase(std::string_view key);
    void clear();
    void erase_prefix(std::string_view prefix);

    [[nodiscard]] Scope scope(std::string prefix)
    {
        if(!prefix.empty()) {
            detail::validate_qualified_key(prefix, "StateStore scope prefix");
        }
        return Scope(this, std::move(prefix));
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return values_.size();
    }

private:
    friend class Subscription;

    struct ObserverEntry {
        std::string key;
        Observer callback;
        bool prefix_match = false;
    };

    static bool matches_prefix(std::string_view key, std::string_view prefix) noexcept;
    void unsubscribe(std::size_t observer_id) noexcept;
    void notify_change(std::string_view key, ChangeKind kind);

    std::unordered_map<std::string, std::any> values_;
    std::unordered_map<std::size_t, ObserverEntry> observers_;
    std::size_t next_observer_id_ = 1;
};

class ComponentScope {
public:
    ComponentScope(
        FrameBuilder& frame_builder,
        StateStore& state_store,
        std::string mount_parent_key,
        std::string component_key_prefix
    )
        : frame_builder_(&frame_builder)
        , state_store_(&state_store)
        , state_scope_(state_store.scope(component_key_prefix))
        , mount_parent_key_(std::move(mount_parent_key))
        , component_key_prefix_(std::move(component_key_prefix))
    {
        detail::validate_qualified_key(
            mount_parent_key_,
            "ComponentScope mount_parent_key"
        );
        detail::validate_qualified_key(
            component_key_prefix_,
            "ComponentScope component_key_prefix"
        );
    }

    [[nodiscard]] std::string node_key(std::string_view local_key) const
    {
        detail::validate_key_segment(local_key, "ComponentScope local_key");

        std::string full_key;
        full_key.reserve(component_key_prefix_.size() + local_key.size() + 1U);
        full_key.append(component_key_prefix_);
        full_key.push_back(detail::k_scope_key_separator);
        full_key.append(local_key);
        return full_key;
    }

    [[nodiscard]] std::string parent_key(std::string_view local_parent_key = {}) const
    {
        if(local_parent_key.empty()) {
            return mount_parent_key_;
        }
        return node_key(local_parent_key);
    }

    [[nodiscard]] std::string auto_local_key(
        std::string_view key_prefix = "node",
        std::size_t salt = 0U,
        const std::source_location& location = std::source_location::current()
    ) const
    {
        return detail::make_auto_key_segment(key_prefix, salt, location);
    }

    [[nodiscard]] std::string_view mount_parent_key() const noexcept
    {
        return mount_parent_key_;
    }

    [[nodiscard]] std::string_view key_prefix() const noexcept
    {
        return component_key_prefix_;
    }

    [[nodiscard]] StateStore::Scope& state() noexcept
    {
        return state_scope_;
    }

    [[nodiscard]] const StateStore::Scope& state() const noexcept
    {
        return state_scope_;
    }

    [[nodiscard]] ComponentScope mount(
        std::string local_component_key,
        std::string local_mount_parent_key = {}
    ) const
    {
        detail::validate_key_segment(
            local_component_key,
            "ComponentScope local_component_key"
        );

        const std::string resolved_parent = local_mount_parent_key.empty()
            ? mount_parent_key_
            : node_key(local_mount_parent_key);
        return ComponentScope(
            *frame_builder_,
            *state_store_,
            resolved_parent,
            node_key(local_component_key)
        );
    }

    template<typename Component>
    void mount_component(
        std::string local_component_key,
        std::string local_mount_parent_key,
        Component&& component
    ) const
    {
        ComponentScope child_scope = mount(
            std::move(local_component_key),
            std::move(local_mount_parent_key)
        );
        std::forward<Component>(component).render(child_scope);
    }

    template<typename Component>
    void mount_component(
        std::string local_component_key,
        Component&& component
    ) const
    {
        mount_component(
            std::move(local_component_key),
            {},
            std::forward<Component>(component)
        );
    }

    template<typename RenderFn>
    void mount_invoke(
        std::string local_component_key,
        std::string local_mount_parent_key,
        RenderFn&& render_fn
    ) const
    {
        ComponentScope child_scope = mount(
            std::move(local_component_key),
            std::move(local_mount_parent_key)
        );
        std::invoke(std::forward<RenderFn>(render_fn), child_scope);
    }

    template<typename RenderFn>
    void mount_invoke(std::string local_component_key, RenderFn&& render_fn) const
    {
        mount_invoke(
            std::move(local_component_key),
            {},
            std::forward<RenderFn>(render_fn)
        );
    }

    FrameBuilder::Entry add(
        std::string local_parent_key,
        std::string local_key,
        NodeKind kind
    )
    {
        return frame_builder_->add(
            parent_key(local_parent_key),
            node_key(local_key),
            kind
        );
    }

    FrameBuilder::Entry add(std::string local_key, NodeKind kind)
    {
        return add({}, std::move(local_key), kind);
    }

    FrameBuilder::Entry panel(std::string local_parent_key, std::string local_key)
    {
        return frame_builder_->panel(
            parent_key(local_parent_key),
            node_key(local_key)
        );
    }

    FrameBuilder::Entry panel(
        std::string local_parent_key,
        const std::source_location& location = std::source_location::current()
    )
    {
        return panel(
            std::move(local_parent_key),
            resolve_local_key_or_auto({}, "panel", location)
        );
    }

    FrameBuilder::Entry panel(
        const std::source_location& location = std::source_location::current()
    )
    {
        return panel({}, location);
    }

    template<detail::LocalKeyType LocalKey>
    FrameBuilder::Entry panel()
    {
        return panel({}, std::string(LocalKey::value));
    }

    FrameBuilder::Entry view(std::string local_parent_key, std::string local_key)
    {
        return frame_builder_->view(
            parent_key(local_parent_key),
            node_key(local_key)
        );
    }

    FrameBuilder::Entry view(
        std::string local_parent_key,
        const std::source_location& location = std::source_location::current()
    )
    {
        return view(
            std::move(local_parent_key),
            resolve_local_key_or_auto({}, "view", location)
        );
    }

    FrameBuilder::Entry view(
        const std::source_location& location = std::source_location::current()
    )
    {
        return view({}, location);
    }

    template<detail::LocalKeyType LocalKey>
    FrameBuilder::Entry view()
    {
        return view({}, std::string(LocalKey::value));
    }

    FrameBuilder::Entry column(
        std::string local_parent_key,
        std::string local_key,
        float gap = 6.0f,
        float padding = 8.0f
    )
    {
        return frame_builder_->column(
            parent_key(local_parent_key),
            node_key(local_key),
            gap,
            padding
        );
    }

    FrameBuilder::Entry column(
        std::string local_parent_key,
        std::string local_key,
        std::initializer_list<AxisTrack> tracks,
        float gap = 6.0f,
        float padding = 8.0f
    )
    {
        return frame_builder_->column(
            parent_key(local_parent_key),
            node_key(local_key),
            tracks,
            gap,
            padding
        );
    }

    FrameBuilder::Entry column(
        std::string local_parent_key,
        float gap = 6.0f,
        float padding = 8.0f,
        const std::source_location& location = std::source_location::current()
    )
    {
        return column(
            std::move(local_parent_key),
            resolve_local_key_or_auto({}, "column", location),
            gap,
            padding
        );
    }

    FrameBuilder::Entry column(
        float gap = 6.0f,
        float padding = 8.0f,
        const std::source_location& location = std::source_location::current()
    )
    {
        return column({}, gap, padding, location);
    }

    template<detail::LocalKeyType LocalKey>
    FrameBuilder::Entry column(float gap = 6.0f, float padding = 8.0f)
    {
        return column({}, std::string(LocalKey::value), gap, padding);
    }

    FrameBuilder::Entry column(
        std::string local_parent_key,
        std::initializer_list<AxisTrack> tracks,
        float gap = 6.0f,
        float padding = 8.0f,
        const std::source_location& location = std::source_location::current()
    )
    {
        return column(
            std::move(local_parent_key),
            resolve_local_key_or_auto({}, "column", location),
            tracks,
            gap,
            padding
        );
    }

    FrameBuilder::Entry column(
        std::initializer_list<AxisTrack> tracks,
        float gap = 6.0f,
        float padding = 8.0f,
        const std::source_location& location = std::source_location::current()
    )
    {
        return column({}, tracks, gap, padding, location);
    }

    template<detail::LocalKeyType ParentKey, detail::LocalKeyType LocalKey>
    FrameBuilder::Entry column(float gap = 6.0f, float padding = 8.0f)
    {
        return column(
            std::string(ParentKey::value),
            std::string(LocalKey::value),
            gap,
            padding
        );
    }

    FrameBuilder::Entry row(
        std::string local_parent_key,
        std::string local_key,
        float gap = 6.0f,
        float padding = 8.0f
    )
    {
        return frame_builder_->row(
            parent_key(local_parent_key),
            node_key(local_key),
            gap,
            padding
        );
    }

    FrameBuilder::Entry row(
        std::string local_parent_key,
        std::string local_key,
        std::initializer_list<AxisTrack> tracks,
        float gap = 6.0f,
        float padding = 8.0f
    )
    {
        return frame_builder_->row(
            parent_key(local_parent_key),
            node_key(local_key),
            tracks,
            gap,
            padding
        );
    }

    FrameBuilder::Entry row(
        std::string local_parent_key,
        float gap = 6.0f,
        float padding = 8.0f,
        const std::source_location& location = std::source_location::current()
    )
    {
        return row(
            std::move(local_parent_key),
            resolve_local_key_or_auto({}, "row", location),
            gap,
            padding
        );
    }

    FrameBuilder::Entry row(
        float gap = 6.0f,
        float padding = 8.0f,
        const std::source_location& location = std::source_location::current()
    )
    {
        return row({}, gap, padding, location);
    }

    template<detail::LocalKeyType LocalKey>
    FrameBuilder::Entry row(float gap = 6.0f, float padding = 8.0f)
    {
        return row({}, std::string(LocalKey::value), gap, padding);
    }

    FrameBuilder::Entry row(
        std::string local_parent_key,
        std::initializer_list<AxisTrack> tracks,
        float gap = 6.0f,
        float padding = 8.0f,
        const std::source_location& location = std::source_location::current()
    )
    {
        return row(
            std::move(local_parent_key),
            resolve_local_key_or_auto({}, "row", location),
            tracks,
            gap,
            padding
        );
    }

    FrameBuilder::Entry row(
        std::initializer_list<AxisTrack> tracks,
        float gap = 6.0f,
        float padding = 8.0f,
        const std::source_location& location = std::source_location::current()
    )
    {
        return row({}, tracks, gap, padding, location);
    }

    template<detail::LocalKeyType ParentKey, detail::LocalKeyType LocalKey>
    FrameBuilder::Entry row(float gap = 6.0f, float padding = 8.0f)
    {
        return row(
            std::string(ParentKey::value),
            std::string(LocalKey::value),
            gap,
            padding
        );
    }

    FrameBuilder::Entry grid(
        std::string local_parent_key,
        std::string local_key,
        std::size_t auto_columns = 1U,
        float gap = 6.0f,
        float padding = 8.0f
    )
    {
        return frame_builder_->grid(
            parent_key(local_parent_key),
            node_key(local_key),
            auto_columns,
            gap,
            padding
        );
    }

    FrameBuilder::Entry grid(
        std::string local_parent_key,
        std::string local_key,
        std::initializer_list<AxisTrack> columns,
        float gap = 6.0f,
        float padding = 8.0f
    )
    {
        return frame_builder_->grid(
            parent_key(local_parent_key),
            node_key(local_key),
            columns,
            gap,
            padding
        );
    }

    FrameBuilder::Entry grid(
        std::string local_parent_key,
        std::size_t auto_columns = 1U,
        float gap = 6.0f,
        float padding = 8.0f,
        const std::source_location& location = std::source_location::current()
    )
    {
        return grid(
            std::move(local_parent_key),
            resolve_local_key_or_auto({}, "grid", location),
            auto_columns,
            gap,
            padding
        );
    }

    FrameBuilder::Entry grid(
        std::size_t auto_columns = 1U,
        float gap = 6.0f,
        float padding = 8.0f,
        const std::source_location& location = std::source_location::current()
    )
    {
        return grid({}, auto_columns, gap, padding, location);
    }

    template<detail::LocalKeyType LocalKey>
    FrameBuilder::Entry grid(
        std::size_t auto_columns = 1U,
        float gap = 6.0f,
        float padding = 8.0f
    )
    {
        return grid({}, std::string(LocalKey::value), auto_columns, gap, padding);
    }

    FrameBuilder::Entry grid(
        std::string local_parent_key,
        std::initializer_list<AxisTrack> columns,
        float gap = 6.0f,
        float padding = 8.0f,
        const std::source_location& location = std::source_location::current()
    )
    {
        return grid(
            std::move(local_parent_key),
            resolve_local_key_or_auto({}, "grid", location),
            columns,
            gap,
            padding
        );
    }

    FrameBuilder::Entry grid(
        std::initializer_list<AxisTrack> columns,
        float gap = 6.0f,
        float padding = 8.0f,
        const std::source_location& location = std::source_location::current()
    )
    {
        return grid({}, columns, gap, padding, location);
    }

    template<detail::LocalKeyType ParentKey, detail::LocalKeyType LocalKey>
    FrameBuilder::Entry grid(
        std::size_t auto_columns = 1U,
        float gap = 6.0f,
        float padding = 8.0f
    )
    {
        return grid(
            std::string(ParentKey::value),
            std::string(LocalKey::value),
            auto_columns,
            gap,
            padding
        );
    }

    FrameBuilder::Entry label(
        std::string local_parent_key,
        std::string local_key,
        std::string text
    )
    {
        return frame_builder_->label(
            parent_key(local_parent_key),
            node_key(local_key),
            std::move(text)
        );
    }

    FrameBuilder::Entry label(
        std::string local_parent_key,
        std::string text,
        const std::source_location& location = std::source_location::current()
    )
    {
        return label(
            std::move(local_parent_key),
            resolve_local_key_or_auto({}, "label", location),
            std::move(text)
        );
    }

    FrameBuilder::Entry label(
        std::string text,
        const std::source_location& location = std::source_location::current()
    )
    {
        return label({}, std::move(text), location);
    }

    template<detail::LocalKeyType LocalKey>
    FrameBuilder::Entry label(std::string text)
    {
        return label({}, std::string(LocalKey::value), std::move(text));
    }

    template<detail::LocalKeyType ParentKey, detail::LocalKeyType LocalKey>
    FrameBuilder::Entry label(std::string text)
    {
        return label(
            std::string(ParentKey::value),
            std::string(LocalKey::value),
            std::move(text)
        );
    }

    FrameBuilder::Entry image(
        std::string local_parent_key,
        std::string local_key,
        std::string image_source
    )
    {
        return frame_builder_->image(
            parent_key(local_parent_key),
            node_key(local_key),
            std::move(image_source)
        );
    }

    FrameBuilder::Entry image(
        std::string local_parent_key,
        std::string image_source,
        const std::source_location& location = std::source_location::current()
    )
    {
        return image(
            std::move(local_parent_key),
            resolve_local_key_or_auto({}, "image", location),
            std::move(image_source)
        );
    }

    FrameBuilder::Entry image(
        std::string image_source,
        const std::source_location& location = std::source_location::current()
    )
    {
        return image({}, std::move(image_source), location);
    }

    FrameBuilder::Entry image_asset(
        std::string local_parent_key,
        std::string local_key,
        std::string image_asset_id
    )
    {
        return frame_builder_->image_asset(
            parent_key(local_parent_key),
            node_key(local_key),
            std::move(image_asset_id)
        );
    }

    FrameBuilder::Entry image_asset(
        std::string local_parent_key,
        std::string image_asset_id,
        const std::source_location& location = std::source_location::current()
    )
    {
        return image_asset(
            std::move(local_parent_key),
            resolve_local_key_or_auto({}, "image_asset", location),
            std::move(image_asset_id)
        );
    }

    FrameBuilder::Entry image_asset(
        std::string image_asset_id,
        const std::source_location& location = std::source_location::current()
    )
    {
        return image_asset({}, std::move(image_asset_id), location);
    }

    FrameBuilder::Entry button(
        std::string local_parent_key,
        std::string local_key,
        std::string text
    )
    {
        return frame_builder_->button(
            parent_key(local_parent_key),
            node_key(local_key),
            std::move(text)
        );
    }

    FrameBuilder::Entry button(
        std::string local_parent_key,
        std::string text,
        const std::source_location& location = std::source_location::current()
    )
    {
        return button(
            std::move(local_parent_key),
            resolve_local_key_or_auto({}, "button", location),
            std::move(text)
        );
    }

    FrameBuilder::Entry button(
        std::string text,
        const std::source_location& location = std::source_location::current()
    )
    {
        return button({}, std::move(text), location);
    }

    template<detail::LocalKeyType LocalKey>
    FrameBuilder::Entry button(std::string text)
    {
        return button({}, std::string(LocalKey::value), std::move(text));
    }

    template<detail::LocalKeyType ParentKey, detail::LocalKeyType LocalKey>
    FrameBuilder::Entry button(std::string text)
    {
        return button(
            std::string(ParentKey::value),
            std::string(LocalKey::value),
            std::move(text)
        );
    }

    FrameBuilder::Entry text_edit(
        std::string local_parent_key,
        std::string local_key,
        std::string value_utf8
    )
    {
        return frame_builder_->text_edit(
            parent_key(local_parent_key),
            node_key(local_key),
            std::move(value_utf8)
        );
    }

    FrameBuilder::Entry text_edit(
        std::string local_parent_key,
        std::string value_utf8,
        const std::source_location& location = std::source_location::current()
    )
    {
        return text_edit(
            std::move(local_parent_key),
            resolve_local_key_or_auto({}, "text_edit", location),
            std::move(value_utf8)
        );
    }

    FrameBuilder::Entry text_edit(
        std::string value_utf8,
        const std::source_location& location = std::source_location::current()
    )
    {
        return text_edit({}, std::move(value_utf8), location);
    }

    template<detail::LocalKeyType LocalKey>
    FrameBuilder::Entry text_edit(std::string value_utf8)
    {
        return text_edit({}, std::string(LocalKey::value), std::move(value_utf8));
    }

    template<detail::LocalKeyType ParentKey, detail::LocalKeyType LocalKey>
    FrameBuilder::Entry text_edit(std::string value_utf8)
    {
        return text_edit(
            std::string(ParentKey::value),
            std::string(LocalKey::value),
            std::move(value_utf8)
        );
    }

    FrameBuilder::Entry custom(std::string local_parent_key, std::string local_key)
    {
        return frame_builder_->custom(
            parent_key(local_parent_key),
            node_key(local_key)
        );
    }

    FrameBuilder::Entry custom(
        std::string local_parent_key,
        const std::source_location& location = std::source_location::current()
    )
    {
        return custom(
            std::move(local_parent_key),
            resolve_local_key_or_auto({}, "custom", location)
        );
    }

    FrameBuilder::Entry custom(
        const std::source_location& location = std::source_location::current()
    )
    {
        return custom({}, location);
    }

private:
    [[nodiscard]] std::string resolve_local_key_or_auto(
        std::string local_key,
        std::string_view auto_key_prefix,
        const std::source_location& location
    )
    {
        if(!local_key.empty()) {
            detail::validate_key_segment(local_key, "ComponentScope local_key");
            return std::move(local_key);
        }

        const std::uint64_t location_hash = detail::hash_source_location(location);
        std::size_t& next_salt = auto_key_salts_by_location_[location_hash];
        return auto_local_key(auto_key_prefix, next_salt++, location);
    }

    FrameBuilder* frame_builder_ = nullptr;
    StateStore* state_store_ = nullptr;
    StateStore::Scope state_scope_;
    std::string mount_parent_key_;
    std::string component_key_prefix_;
    std::unordered_map<std::uint64_t, std::size_t> auto_key_salts_by_location_{};
};

} // namespace guinevere::ui
