#pragma once

#include <string>
#include <string_view>

#include <memory>

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/middlewares/configuration.hpp>
#include <userver/server/middlewares/http_middleware_base.hpp>

#include "storage/recipe_storage.hpp"

namespace recipe_service {

class AuthMiddleware final : public userver::server::middlewares::HttpMiddlewareBase {
public:
    static constexpr std::string_view kName = "recipe-auth-middleware";

    AuthMiddleware(const userver::server::handlers::HttpHandlerBase& handler, RecipeStorage& storage);

private:
    void HandleRequest(
        userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context
    ) const override;

    RecipeStorage& storage_;
};

class AuthMiddlewareFactory final : public userver::server::middlewares::HttpMiddlewareFactoryBase {
public:
    static constexpr std::string_view kName = AuthMiddleware::kName;

    AuthMiddlewareFactory(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

private:
    std::unique_ptr<userver::server::middlewares::HttpMiddlewareBase> Create(
        const userver::server::handlers::HttpHandlerBase& handler,
        userver::yaml_config::YamlConfig middleware_config
    ) const override;

    RecipeStorage& storage_;
};

class AuthHandlerPipelineBuilder final : public userver::server::middlewares::HandlerPipelineBuilder {
public:
    static constexpr std::string_view kName = "auth-handler-pipeline-builder";

    using userver::server::middlewares::HandlerPipelineBuilder::HandlerPipelineBuilder;

    userver::server::middlewares::MiddlewaresList BuildPipeline(
        userver::server::middlewares::MiddlewaresList server_middleware_pipeline
    ) const override;
};

}  // namespace recipe_service
