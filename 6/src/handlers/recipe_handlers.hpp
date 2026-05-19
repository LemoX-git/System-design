#pragma once

#include <string>
#include <string_view>

#include "handlers/base_handler.hpp"

namespace recipe_service {

class CreateRecipeHandler final : public StorageAwareHandler {
public:
    static constexpr std::string_view kName = "handler-create-recipe";

    using StorageAwareHandler::StorageAwareHandler;

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context
    ) const override;
};

class ListRecipesHandler final : public StorageAwareHandler {
public:
    static constexpr std::string_view kName = "handler-list-recipes";

    using StorageAwareHandler::StorageAwareHandler;

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context
    ) const override;
};

class AddIngredientHandler final : public StorageAwareHandler {
public:
    static constexpr std::string_view kName = "handler-add-ingredient";

    using StorageAwareHandler::StorageAwareHandler;

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context
    ) const override;
};

class ListIngredientsHandler final : public StorageAwareHandler {
public:
    static constexpr std::string_view kName = "handler-list-ingredients";

    using StorageAwareHandler::StorageAwareHandler;

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context
    ) const override;
};

class UserRecipesHandler final : public StorageAwareHandler {
public:
    static constexpr std::string_view kName = "handler-user-recipes";

    using StorageAwareHandler::StorageAwareHandler;

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context
    ) const override;
};

class AddFavoriteHandler final : public StorageAwareHandler {
public:
    static constexpr std::string_view kName = "handler-add-favorite";

    using StorageAwareHandler::StorageAwareHandler;

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context
    ) const override;
};

class FavoritesHandler final : public StorageAwareHandler {
public:
    static constexpr std::string_view kName = "handler-list-favorites";

    using StorageAwareHandler::StorageAwareHandler;

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context
    ) const override;
};

}  // namespace recipe_service
