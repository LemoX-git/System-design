#include "storage/recipe_storage.hpp"

#include <algorithm>

#include <userver/components/component_context.hpp>

#include <fmt/format.h>

#include "utils/string_utils.hpp"

namespace recipe_service {

RecipeStorage::RecipeStorage(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
) : userver::components::ComponentBase(config, context) {
}

std::optional<User> RecipeStorage::RegisterUser(
    const std::string& login,
    const std::string& first_name,
    const std::string& last_name,
    const std::string& password
) {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    if (login.empty() || first_name.empty() || last_name.empty() || password.empty()) {
        return std::nullopt;
    }
    if (user_id_by_login_.count(login) != 0) {
        return std::nullopt;
    }

    User user;
    user.id = next_user_id_++;
    user.login = login;
    user.first_name = first_name;
    user.last_name = last_name;
    user.password = password;

    user_id_by_login_[user.login] = user.id;
    users_[user.id] = user;
    return user;
}

std::optional<std::pair<User, std::string>> RecipeStorage::Login(
    const std::string& login,
    const std::string& password
) {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    const auto login_it = user_id_by_login_.find(login);
    if (login_it == user_id_by_login_.end()) {
        return std::nullopt;
    }

    User& user = users_.at(login_it->second);
    if (user.password != password) {
        return std::nullopt;
    }

    const auto token = fmt::format("token-{}-{}", user.id, next_token_id_++);
    token_to_user_id_[token] = user.id;
    return std::make_pair(user, token);
}

std::optional<User> RecipeStorage::FindUserByLogin(const std::string& login) const {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);
    const auto login_it = user_id_by_login_.find(login);
    if (login_it == user_id_by_login_.end()) {
        return std::nullopt;
    }
    return users_.at(login_it->second);
}

std::vector<User> RecipeStorage::SearchUsersByMask(const std::string& mask) const {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);
    const auto lowered_mask = ToLower(mask);

    std::vector<User> result;
    for (const auto& [id, user] : users_) {
        const auto full_name = ToLower(user.first_name + " " + user.last_name);
        if (full_name.find(lowered_mask) != std::string::npos) {
            result.push_back(user);
        }
    }

    SortUsersById(result);
    return result;
}

std::optional<int> RecipeStorage::ValidateToken(const std::string& token) const {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);
    const auto it = token_to_user_id_.find(token);
    if (it == token_to_user_id_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<Recipe> RecipeStorage::CreateRecipe(
    int author_id,
    const std::string& title,
    const std::string& description
) {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    if (users_.count(author_id) == 0 || title.empty() || description.empty()) {
        return std::nullopt;
    }

    Recipe recipe;
    recipe.id = next_recipe_id_++;
    recipe.author_id = author_id;
    recipe.title = title;
    recipe.description = description;

    recipes_[recipe.id] = recipe;
    return recipe;
}

std::vector<Recipe> RecipeStorage::ListRecipes(const std::optional<std::string>& title_filter) const {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    const auto lowered_filter = title_filter ? ToLower(*title_filter) : std::string{};
    std::vector<Recipe> result;

    for (const auto& [id, recipe] : recipes_) {
        if (!title_filter || ToLower(recipe.title).find(lowered_filter) != std::string::npos) {
            result.push_back(recipe);
        }
    }

    SortRecipesById(result);
    return result;
}

std::optional<Recipe> RecipeStorage::GetRecipe(int recipe_id) const {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);
    const auto it = recipes_.find(recipe_id);
    if (it == recipes_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::pair<RecipeStorage::AddIngredientStatus, std::optional<Ingredient>> RecipeStorage::AddIngredient(
    int user_id,
    int recipe_id,
    const std::string& name,
    const std::string& amount
) {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    auto recipe_it = recipes_.find(recipe_id);
    if (recipe_it == recipes_.end()) {
        return {AddIngredientStatus::kRecipeNotFound, std::nullopt};
    }
    if (recipe_it->second.author_id != user_id) {
        return {AddIngredientStatus::kForbidden, std::nullopt};
    }
    if (name.empty() || amount.empty()) {
        return {AddIngredientStatus::kInvalidInput, std::nullopt};
    }

    Ingredient ingredient;
    ingredient.id = next_ingredient_id_++;
    ingredient.name = name;
    ingredient.amount = amount;

    recipe_it->second.ingredients.push_back(ingredient);
    return {AddIngredientStatus::kAdded, ingredient};
}

std::optional<std::vector<Ingredient>> RecipeStorage::GetIngredients(int recipe_id) const {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);
    const auto it = recipes_.find(recipe_id);
    if (it == recipes_.end()) {
        return std::nullopt;
    }

    auto ingredients = it->second.ingredients;
    SortIngredientsById(ingredients);
    return ingredients;
}

std::vector<Recipe> RecipeStorage::GetRecipesByUser(int user_id) const {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);
    std::vector<Recipe> result;
    for (const auto& [id, recipe] : recipes_) {
        if (recipe.author_id == user_id) {
            result.push_back(recipe);
        }
    }
    SortRecipesById(result);
    return result;
}

RecipeStorage::FavoriteStatus RecipeStorage::AddFavorite(int user_id, int recipe_id) {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    auto user_it = users_.find(user_id);
    if (user_it == users_.end()) {
        return FavoriteStatus::kUserNotFound;
    }
    if (recipes_.count(recipe_id) == 0) {
        return FavoriteStatus::kRecipeNotFound;
    }

    const auto [_, inserted] = user_it->second.favorite_recipe_ids.insert(recipe_id);
    if (!inserted) {
        return FavoriteStatus::kAlreadyExists;
    }
    return FavoriteStatus::kAdded;
}

std::vector<Recipe> RecipeStorage::GetFavorites(int user_id) const {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    std::vector<Recipe> result;
    const auto user_it = users_.find(user_id);
    if (user_it == users_.end()) {
        return result;
    }

    for (const int recipe_id : user_it->second.favorite_recipe_ids) {
        const auto recipe_it = recipes_.find(recipe_id);
        if (recipe_it != recipes_.end()) {
            result.push_back(recipe_it->second);
        }
    }

    SortRecipesById(result);
    return result;
}

void RecipeStorage::SortUsersById(std::vector<User>& users) {
    std::sort(users.begin(), users.end(), [](const User& lhs, const User& rhs) {
        return lhs.id < rhs.id;
    });
}

void RecipeStorage::SortRecipesById(std::vector<Recipe>& recipes) {
    std::sort(recipes.begin(), recipes.end(), [](const Recipe& lhs, const Recipe& rhs) {
        return lhs.id < rhs.id;
    });
}

void RecipeStorage::SortIngredientsById(std::vector<Ingredient>& ingredients) {
    std::sort(ingredients.begin(), ingredients.end(), [](const Ingredient& lhs, const Ingredient& rhs) {
        return lhs.id < rhs.id;
    });
}

}  // namespace recipe_service
