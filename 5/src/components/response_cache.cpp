#include "components/response_cache.hpp"

#include <vector>

#include <userver/components/component_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace recipe_service {

ResponseCache::ResponseCache(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
) : userver::components::ComponentBase(config, context),
    users_ttl_(std::chrono::seconds{config["users-ttl-seconds"].As<int>(300)}),
    recipes_list_ttl_(std::chrono::seconds{config["recipes-list-ttl-seconds"].As<int>(30)}),
    ingredients_ttl_(std::chrono::seconds{config["ingredients-ttl-seconds"].As<int>(60)}),
    user_recipes_ttl_(std::chrono::seconds{config["user-recipes-ttl-seconds"].As<int>(30)}),
    favorites_ttl_(std::chrono::seconds{config["favorites-ttl-seconds"].As<int>(30)}) {
}

userver::yaml_config::Schema ResponseCache::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(R"(
        type: object
        description: In-memory response cache for hot GET endpoints.
        additionalProperties: false
        properties:
            users-ttl-seconds:
                type: integer
                description: TTL for user lookup and search responses.
                default: 300
            recipes-list-ttl-seconds:
                type: integer
                description: TTL for recipe list responses.
                default: 30
            ingredients-ttl-seconds:
                type: integer
                description: TTL for recipe ingredients responses.
                default: 60
            user-recipes-ttl-seconds:
                type: integer
                description: TTL for current user recipes responses.
                default: 30
            favorites-ttl-seconds:
                type: integer
                description: TTL for favorites responses.
                default: 30
    )");
}

std::optional<std::string> ResponseCache::Get(const std::string& key) const {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);
    RemoveExpiredLocked();

    const auto it = entries_.find(key);
    if (it == entries_.end()) {
        return std::nullopt;
    }

    return it->second.value;
}

void ResponseCache::Put(const std::string& key, std::string value, std::chrono::seconds ttl) {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);
    entries_[key] = CacheEntry{
        .value = std::move(value),
        .expires_at = std::chrono::steady_clock::now() + ttl,
    };
}

void ResponseCache::Invalidate(const std::string& key) {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);
    entries_.erase(key);
}

void ResponseCache::InvalidatePrefix(const std::string& prefix) {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    std::vector<std::string> keys_to_erase;
    keys_to_erase.reserve(entries_.size());
    for (const auto& [key, _] : entries_) {
        if (key.rfind(prefix, 0) == 0) {
            keys_to_erase.push_back(key);
        }
    }

    for (const auto& key : keys_to_erase) {
        entries_.erase(key);
    }
}

void ResponseCache::Clear() {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);
    entries_.clear();
}

std::chrono::seconds ResponseCache::GetUsersTtl() const {
    return users_ttl_;
}

std::chrono::seconds ResponseCache::GetRecipesListTtl() const {
    return recipes_list_ttl_;
}

std::chrono::seconds ResponseCache::GetIngredientsTtl() const {
    return ingredients_ttl_;
}

std::chrono::seconds ResponseCache::GetUserRecipesTtl() const {
    return user_recipes_ttl_;
}

std::chrono::seconds ResponseCache::GetFavoritesTtl() const {
    return favorites_ttl_;
}

void ResponseCache::RemoveExpiredLocked() const {
    const auto now = std::chrono::steady_clock::now();

    std::vector<std::string> keys_to_erase;
    keys_to_erase.reserve(entries_.size());
    for (const auto& [key, entry] : entries_) {
        if (entry.expires_at <= now) {
            keys_to_erase.push_back(key);
        }
    }

    for (const auto& key : keys_to_erase) {
        entries_.erase(key);
    }
}

}  // namespace recipe_service
