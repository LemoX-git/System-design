#include "middlewares/auth_middleware.hpp"

#include <string>

#include <userver/components/component_context.hpp>

#include "utils/http_utils.hpp"

namespace recipe_service {

AuthMiddleware::AuthMiddleware(
    const userver::server::handlers::HttpHandlerBase&,
    RecipeStorage& storage
) : storage_(storage) {
}

void AuthMiddleware::HandleRequest(
    userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context
) const {
    const auto& header = request.GetHeader("Authorization");
    constexpr std::string_view kPrefix = "Bearer ";

    if (header.rfind(kPrefix, 0) != 0) {
        request.GetHttpResponse().SetHeader(std::string{"WWW-Authenticate"}, std::string{"Bearer"});
        SetJsonResponseBody(
            request.GetHttpResponse(),
            MakeErrorJson("Missing or invalid Authorization header"),
            HttpStatus::kUnauthorized
        );
        return;
    }

    const auto token = header.substr(kPrefix.size());
    const auto user_id = storage_.ValidateToken(token);
    if (!user_id) {
        request.GetHttpResponse().SetHeader(std::string{"WWW-Authenticate"}, std::string{"Bearer"});
        SetJsonResponseBody(
            request.GetHttpResponse(),
            MakeErrorJson("Invalid bearer token"),
            HttpStatus::kUnauthorized
        );
        return;
    }

    context.SetData(std::string{kUserIdContextKey}, *user_id);
    Next(request, context);
}

AuthMiddlewareFactory::AuthMiddlewareFactory(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
) : userver::server::middlewares::HttpMiddlewareFactoryBase(config, context),
    storage_(context.FindComponent<RecipeStorage>()) {
}

std::unique_ptr<userver::server::middlewares::HttpMiddlewareBase> AuthMiddlewareFactory::Create(
    const userver::server::handlers::HttpHandlerBase& handler,
    userver::yaml_config::YamlConfig
) const {
    return std::make_unique<AuthMiddleware>(handler, storage_);
}

userver::server::middlewares::MiddlewaresList AuthHandlerPipelineBuilder::BuildPipeline(
    userver::server::middlewares::MiddlewaresList server_middleware_pipeline
) const {
    server_middleware_pipeline.emplace_back(std::string{AuthMiddlewareFactory::kName});
    return server_middleware_pipeline;
}

}  // namespace recipe_service
