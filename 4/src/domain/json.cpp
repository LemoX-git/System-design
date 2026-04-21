#include "domain/json.hpp"

#include <userver/formats/common/type.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace recipe_service {

userver::formats::json::Value UserToJson(const User& user) {
    userver::formats::json::ValueBuilder builder(userver::formats::common::Type::kObject);
    builder["id"] = user.id;
    builder["login"] = user.login;
    builder["first_name"] = user.first_name;
    builder["last_name"] = user.last_name;
    return builder.ExtractValue();
}

userver::formats::json::Value IngredientToJson(const Ingredient& ingredient) {
    userver::formats::json::ValueBuilder builder(userver::formats::common::Type::kObject);
    builder["id"] = ingredient.id;
    builder["name"] = ingredient.name;
    builder["amount"] = ingredient.amount;
    return builder.ExtractValue();
}

userver::formats::json::Value RecipeSummaryToJson(const Recipe& recipe) {
    userver::formats::json::ValueBuilder builder(userver::formats::common::Type::kObject);
    builder["id"] = recipe.id;
    builder["author_id"] = recipe.author_id;
    builder["title"] = recipe.title;
    builder["description"] = recipe.description;
    builder["ingredients_count"] = static_cast<int>(recipe.ingredients.size());
    return builder.ExtractValue();
}

userver::formats::json::Value RecipeDetailedToJson(const Recipe& recipe) {
    userver::formats::json::ValueBuilder builder(userver::formats::common::Type::kObject);
    builder["id"] = recipe.id;
    builder["author_id"] = recipe.author_id;
    builder["title"] = recipe.title;
    builder["description"] = recipe.description;

    userver::formats::json::ValueBuilder ingredients(userver::formats::common::Type::kArray);
    for (const auto& ingredient : recipe.ingredients) {
        ingredients.PushBack(IngredientToJson(ingredient));
    }
    builder["ingredients"] = ingredients.ExtractValue();
    return builder.ExtractValue();
}

}  // namespace recipe_service
