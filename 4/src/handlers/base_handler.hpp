#pragma once

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/components/component_context.hpp>

#include "storage/recipe_storage.hpp"

namespace recipe_service {

class StorageAwareHandler : public userver::server::handlers::HttpHandlerBase {
public:
    StorageAwareHandler(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    ) : userver::server::handlers::HttpHandlerBase(config, context),
        storage_(context.FindComponent<RecipeStorage>()) {
    }

protected:
    RecipeStorage& GetStorage() const { return storage_; }

private:
    RecipeStorage& storage_;
};

}  // namespace recipe_service
