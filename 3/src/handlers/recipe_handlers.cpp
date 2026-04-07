#include "handlers/recipe_handlers.hpp"

#include <optional>

#include <userver/formats/common/type.hpp>
#include <userver/formats/json/value_builder.hpp>

#include "domain/json.hpp"
#include "utils/http_utils.hpp"

namespace recipe_service {
namespace {

std::string MakeExceptionResponse(
    const userver::server::http::HttpRequest& request,
    const std::exception& ex
) {
    if (dynamic_cast<const ClientError*>(&ex) != nullptr) {
        return MakeJsonResponse(request, MakeErrorJson(ex.what()), HttpStatus::kBadRequest);
    }
    return MakeJsonResponse(request, MakeErrorJson("Internal server error"), HttpStatus::kInternalServerError);
}

}  // namespace


std::string CreateRecipeHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context
) const {
    try {
        const int author_id = GetAuthorizedUserId(context);
        const auto json = ParseBodyJson(request);

        const auto title = json["title"].As<std::string>("");
        const auto description = json["description"].As<std::string>("");

        if (title.empty() || description.empty()) {
            return MakeJsonResponse(request, MakeErrorJson("Recipe title and description are required"), HttpStatus::kBadRequest);
        }

        const auto recipe = GetStorage().CreateRecipe(author_id, title, description);
        if (!recipe) {
            return MakeJsonResponse(request, MakeErrorJson("Recipe could not be created"), HttpStatus::kBadRequest);
        }

        return MakeJsonResponse(request, RecipeDetailedToJson(*recipe), HttpStatus::kCreated);
    } catch (const std::exception& ex) {
        return MakeExceptionResponse(request, ex);
    }
}

std::string ListRecipesHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&
) const {
    const auto title = request.GetArg("title");
    const std::optional<std::string> title_filter =
        !title.empty() ? std::make_optional(title) : std::nullopt;

    try {
        const auto recipes = GetStorage().ListRecipes(title_filter);
        userver::formats::json::ValueBuilder array(userver::formats::common::Type::kArray);
        for (const auto& recipe : recipes) {
            array.PushBack(RecipeSummaryToJson(recipe));
        }
        return MakeJsonResponse(request, array.ExtractValue());
    } catch (const std::exception& ex) {
        return MakeExceptionResponse(request, ex);
    }
}

std::string AddIngredientHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context
) const {
    try {
        const int user_id = GetAuthorizedUserId(context);
        const int recipe_id = ParseId(request.GetPathArg("recipe_id"), "recipe_id");
        const auto json = ParseBodyJson(request);

        const auto name = json["name"].As<std::string>("");
        const auto amount = json["amount"].As<std::string>("");

        const auto [status, ingredient] = GetStorage().AddIngredient(user_id, recipe_id, name, amount);
        switch (status) {
            case RecipeStorage::AddIngredientStatus::kAdded:
                return MakeJsonResponse(request, IngredientToJson(*ingredient), HttpStatus::kCreated);
            case RecipeStorage::AddIngredientStatus::kRecipeNotFound:
                return MakeJsonResponse(request, MakeErrorJson("Recipe not found"), HttpStatus::kNotFound);
            case RecipeStorage::AddIngredientStatus::kForbidden:
                return MakeJsonResponse(
                    request,
                    MakeErrorJson("Only the recipe author can add ingredients"),
                    HttpStatus::kForbidden
                );
            case RecipeStorage::AddIngredientStatus::kInvalidInput:
                return MakeJsonResponse(
                    request,
                    MakeErrorJson("Ingredient name and amount are required"),
                    HttpStatus::kBadRequest
                );
        }

        return MakeJsonResponse(request, MakeErrorJson("Unexpected status"), HttpStatus::kInternalServerError);
    } catch (const std::exception& ex) {
        return MakeExceptionResponse(request, ex);
    }
}

std::string ListIngredientsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&
) const {
    try {
        const int recipe_id = ParseId(request.GetPathArg("recipe_id"), "recipe_id");
        const auto ingredients = GetStorage().GetIngredients(recipe_id);
        if (!ingredients) {
            return MakeJsonResponse(request, MakeErrorJson("Recipe not found"), HttpStatus::kNotFound);
        }

        userver::formats::json::ValueBuilder array(userver::formats::common::Type::kArray);
        for (const auto& ingredient : *ingredients) {
            array.PushBack(IngredientToJson(ingredient));
        }
        return MakeJsonResponse(request, array.ExtractValue());
    } catch (const std::exception& ex) {
        return MakeExceptionResponse(request, ex);
    }
}

std::string UserRecipesHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context
) const {
    try {
        const int user_id = GetAuthorizedUserId(context);
        const auto recipes = GetStorage().GetRecipesByUser(user_id);

        userver::formats::json::ValueBuilder array(userver::formats::common::Type::kArray);
        for (const auto& recipe : recipes) {
            array.PushBack(RecipeSummaryToJson(recipe));
        }
        return MakeJsonResponse(request, array.ExtractValue());
    } catch (const std::exception& ex) {
        return MakeExceptionResponse(request, ex);
    }
}

std::string AddFavoriteHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context
) const {
    try {
        const int user_id = GetAuthorizedUserId(context);
        const int recipe_id = ParseId(request.GetPathArg("recipe_id"), "recipe_id");

        switch (GetStorage().AddFavorite(user_id, recipe_id)) {
            case RecipeStorage::FavoriteStatus::kAdded: {
                userver::formats::json::ValueBuilder builder(userver::formats::common::Type::kObject);
                builder["message"] = "Recipe added to favorites";
                builder["recipe_id"] = recipe_id;
                return MakeJsonResponse(request, builder.ExtractValue(), HttpStatus::kCreated);
            }
            case RecipeStorage::FavoriteStatus::kAlreadyExists:
                return MakeJsonResponse(request, MakeErrorJson("Recipe is already in favorites"), HttpStatus::kConflict);
            case RecipeStorage::FavoriteStatus::kRecipeNotFound:
                return MakeJsonResponse(request, MakeErrorJson("Recipe not found"), HttpStatus::kNotFound);
            case RecipeStorage::FavoriteStatus::kUserNotFound:
                return MakeJsonResponse(request, MakeErrorJson("User not found"), HttpStatus::kNotFound);
        }

        return MakeJsonResponse(request, MakeErrorJson("Unexpected status"), HttpStatus::kInternalServerError);
    } catch (const std::exception& ex) {
        return MakeExceptionResponse(request, ex);
    }
}

std::string FavoritesHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context
) const {
    try {
        const int user_id = GetAuthorizedUserId(context);
        const auto favorites = GetStorage().GetFavorites(user_id);

        userver::formats::json::ValueBuilder array(userver::formats::common::Type::kArray);
        for (const auto& recipe : favorites) {
            array.PushBack(RecipeSummaryToJson(recipe));
        }
        return MakeJsonResponse(request, array.ExtractValue());
    } catch (const std::exception& ex) {
        return MakeExceptionResponse(request, ex);
    }
}

}  // namespace recipe_service
