#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <userver/components/component_base.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/yaml_config/schema.hpp>

namespace recipe_service {

class ResponseCache final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "response-cache";

    ResponseCache(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

    static userver::yaml_config::Schema GetStaticConfigSchema();

    std::optional<std::string> Get(const std::string& key) const;
    void Put(const std::string& key, std::string value, std::chrono::seconds ttl);
    void Invalidate(const std::string& key);
    void InvalidatePrefix(const std::string& prefix);
    void Clear();

    std::chrono::seconds GetUsersTtl() const;
    std::chrono::seconds GetRecipesListTtl() const;
    std::chrono::seconds GetIngredientsTtl() const;
    std::chrono::seconds GetUserRecipesTtl() const;
    std::chrono::seconds GetFavoritesTtl() const;

private:
    struct CacheEntry {
        std::string value;
        std::chrono::steady_clock::time_point expires_at;
    };

    void RemoveExpiredLocked() const;

    mutable userver::engine::Mutex mutex_;
    mutable std::unordered_map<std::string, CacheEntry> entries_;

    std::chrono::seconds users_ttl_;
    std::chrono::seconds recipes_list_ttl_;
    std::chrono::seconds ingredients_ttl_;
    std::chrono::seconds user_recipes_ttl_;
    std::chrono::seconds favorites_ttl_;
};

}  // namespace recipe_service
