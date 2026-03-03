#include <guinevere/ui/runtime.hpp>

namespace guinevere::ui {

bool StateStore::matches_prefix(std::string_view key, std::string_view prefix) noexcept
{
    if(prefix.empty()) {
        return true;
    }

    if(key == prefix) {
        return true;
    }

    if(key.size() <= prefix.size()) {
        return false;
    }

    return key.rfind(prefix, 0) == 0 && key[prefix.size()] == '.';
}

StateStore::Subscription StateStore::observe(std::string key, Observer observer)
{
    if(!observer || key.empty()) {
        return Subscription{};
    }

    const std::size_t observer_id = next_observer_id_++;
    observers_.emplace(
        observer_id,
        ObserverEntry{std::move(key), std::move(observer), false}
    );
    return Subscription(this, observer_id);
}

StateStore::Subscription StateStore::observe_prefix(std::string prefix, Observer observer)
{
    if(!observer) {
        return Subscription{};
    }

    const std::size_t observer_id = next_observer_id_++;
    observers_.emplace(
        observer_id,
        ObserverEntry{std::move(prefix), std::move(observer), true}
    );
    return Subscription(this, observer_id);
}

void StateStore::erase(std::string_view key)
{
    const std::string owned_key(key);
    if(values_.erase(owned_key) == 0U) {
        return;
    }

    notify_change(owned_key, ChangeKind::Erase);
}

void StateStore::clear()
{
    if(values_.empty()) {
        return;
    }

    std::vector<std::string> erased_keys;
    erased_keys.reserve(values_.size());
    for(const auto& kv : values_) {
        erased_keys.push_back(kv.first);
    }

    values_.clear();
    for(const auto& key : erased_keys) {
        notify_change(key, ChangeKind::Erase);
    }
}

void StateStore::erase_prefix(std::string_view prefix)
{
    if(prefix.empty()) {
        clear();
        return;
    }

    std::vector<std::string> erased_keys;
    erased_keys.reserve(values_.size());
    for(const auto& kv : values_) {
        if(matches_prefix(kv.first, prefix)) {
            erased_keys.push_back(kv.first);
        }
    }

    for(const auto& key : erased_keys) {
        values_.erase(key);
        notify_change(key, ChangeKind::Erase);
    }
}

void StateStore::unsubscribe(std::size_t observer_id) noexcept
{
    observers_.erase(observer_id);
}

void StateStore::notify_change(std::string_view key, ChangeKind kind)
{
    if(observers_.empty()) {
        return;
    }

    std::vector<Observer> callbacks;
    callbacks.reserve(observers_.size());

    for(const auto& kv : observers_) {
        const ObserverEntry& entry = kv.second;
        const bool matches = entry.prefix_match
            ? matches_prefix(key, entry.key)
            : key == entry.key;
        if(matches && entry.callback) {
            callbacks.push_back(entry.callback);
        }
    }

    const ChangeEvent event{key, kind};
    for(const auto& callback : callbacks) {
        callback(event);
    }
}

} // namespace guinevere::ui
