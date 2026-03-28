#pragma once

#include <optional>
#include <string_view>
#include <utility>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/engine/mutex.hpp>

#include "domain/models.hpp"

namespace recipe_service {

class RecipeStorage final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "recipe-storage";

    RecipeStorage(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

    std::optional<User> RegisterUser(
        const std::string& login,
        const std::string& first_name,
        const std::string& last_name,
        const std::string& password
    );

    std::optional<std::pair<User, std::string>> Login(
        const std::string& login,
        const std::string& password
    );

    std::optional<User> FindUserByLogin(const std::string& login) const;
    std::vector<User> SearchUsersByMask(const std::string& mask) const;
    std::optional<int> ValidateToken(const std::string& token) const;

    std::optional<Recipe> CreateRecipe(
        int author_id,
        const std::string& title,
        const std::string& description
    );

    std::vector<Recipe> ListRecipes(const std::optional<std::string>& title_filter) const;
    std::optional<Recipe> GetRecipe(int recipe_id) const;

    enum class AddIngredientStatus {
        kAdded,
        kRecipeNotFound,
        kForbidden,
        kInvalidInput,
    };

    std::pair<AddIngredientStatus, std::optional<Ingredient>> AddIngredient(
        int user_id,
        int recipe_id,
        const std::string& name,
        const std::string& amount
    );

    std::optional<std::vector<Ingredient>> GetIngredients(int recipe_id) const;
    std::vector<Recipe> GetRecipesByUser(int user_id) const;

    enum class FavoriteStatus {
        kAdded,
        kAlreadyExists,
        kRecipeNotFound,
        kUserNotFound,
    };

    FavoriteStatus AddFavorite(int user_id, int recipe_id);
    std::vector<Recipe> GetFavorites(int user_id) const;

private:
    static void SortUsersById(std::vector<User>& users);
    static void SortRecipesById(std::vector<Recipe>& recipes);
    static void SortIngredientsById(std::vector<Ingredient>& ingredients);

    mutable userver::engine::Mutex mutex_;
    int next_user_id_{1};
    int next_recipe_id_{1};
    int next_ingredient_id_{1};
    int next_token_id_{1};

    std::unordered_map<int, User> users_;
    std::unordered_map<std::string, int> user_id_by_login_;
    std::unordered_map<std::string, int> token_to_user_id_;
    std::unordered_map<int, Recipe> recipes_;
};

}  // namespace recipe_service
