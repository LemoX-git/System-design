#include <string>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>

#include "handlers/auth_handlers.hpp"
#include "handlers/recipe_handlers.hpp"
#include "handlers/user_handlers.hpp"
#include "middlewares/auth_middleware.hpp"
#include "storage/recipe_storage.hpp"

int main(int argc, char* argv[]) {
    const auto component_list =
    userver::components::MinimalServerComponentList()
        .Append<recipe_service::RecipeStorage>()
        .Append<recipe_service::AuthMiddlewareFactory>()
        .Append<recipe_service::AuthHandlerPipelineBuilder>(
            std::string{recipe_service::AuthHandlerPipelineBuilder::kName}
        )
        .Append<recipe_service::RegisterHandler>()
        .Append<recipe_service::LoginHandler>()
        .Append<recipe_service::UserByLoginHandler>()
        .Append<recipe_service::UserSearchHandler>()
        .Append<recipe_service::CreateRecipeHandler>()
        .Append<recipe_service::ListRecipesHandler>()
        .Append<recipe_service::AddIngredientHandler>()
        .Append<recipe_service::ListIngredientsHandler>()
        .Append<recipe_service::UserRecipesHandler>()
        .Append<recipe_service::AddFavoriteHandler>()
        .Append<recipe_service::FavoritesHandler>()
        .Append<userver::components::TestsuiteSupport>()
        .AppendComponentList(userver::clients::http::ComponentList())
        .Append<userver::clients::dns::Component>()
        .Append<userver::server::handlers::TestsControl>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
