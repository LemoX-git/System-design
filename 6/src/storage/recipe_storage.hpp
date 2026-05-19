#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/yaml_config/schema.hpp>

#include "domain/models.hpp"

struct pg_conn;
typedef struct pg_conn PGconn;

namespace recipe_service {

class RecipeStorage final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "recipe-storage";

    RecipeStorage(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

    ~RecipeStorage() override;

    static userver::yaml_config::Schema GetStaticConfigSchema();

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
    void ConnectLocked();
    void InitializeSchemaLocked();

    mutable userver::engine::Mutex mutex_;
    std::string dbconnection_;
    std::string schema_path_;
    PGconn* connection_{nullptr};
};

}  // namespace recipe_service
