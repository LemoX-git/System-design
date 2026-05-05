#pragma once

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/components/component_context.hpp>

#include "components/rate_limiter.hpp"
#include "components/response_cache.hpp"
#include "storage/recipe_storage.hpp"

namespace recipe_service {

class StorageAwareHandler : public userver::server::handlers::HttpHandlerBase {
public:
    StorageAwareHandler(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    ) : userver::server::handlers::HttpHandlerBase(config, context),
        storage_(context.FindComponent<RecipeStorage>()),
        cache_(context.FindComponent<ResponseCache>()),
        rate_limiter_(context.FindComponent<RateLimiter>()) {
    }

protected:
    RecipeStorage& GetStorage() const { return storage_; }
    ResponseCache& GetCache() const { return cache_; }
    RateLimiter& GetRateLimiter() const { return rate_limiter_; }

private:
    RecipeStorage& storage_;
    ResponseCache& cache_;
    RateLimiter& rate_limiter_;
};

}  // namespace recipe_service
