#pragma once

#include <userver/formats/json/value.hpp>

#include "domain/models.hpp"

namespace recipe_service {

userver::formats::json::Value UserToJson(const User& user);
userver::formats::json::Value IngredientToJson(const Ingredient& ingredient);
userver::formats::json::Value RecipeSummaryToJson(const Recipe& recipe);
userver::formats::json::Value RecipeDetailedToJson(const Recipe& recipe);

}  // namespace recipe_service
