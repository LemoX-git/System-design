#include "storage/recipe_storage.hpp"

#include <algorithm>
#include <chrono>
#include <optional>
#include <string>
#include <stdexcept>
#include <vector>

#include <userver/components/component_context.hpp>
#include <userver/formats/bson.hpp>
#include <userver/formats/bson/value_builder.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/storages/mongo/options.hpp>

#include "utils/string_utils.hpp"

namespace recipe_service {
namespace {

constexpr std::string_view kUsersCollection = "users";
constexpr std::string_view kRecipesCollection = "recipes";
constexpr std::string_view kCountersCollection = "counters";

std::string HashPassword(const std::string& password) {
    // Учебный, но детерминированный вариант: один и тот же пароль
    // одинаково кодируется и в C++, и в seed-скриптах Mongo.
    return "pw$" + password;
}

}  // namespace

RecipeStorage::RecipeStorage(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
) : userver::components::ComponentBase(config, context),
    pool_(context.FindComponent<userver::components::Mongo>(std::string{kMongoComponentName}).GetPool()) {
}

std::optional<User> RecipeStorage::RegisterUser(
    const std::string& login,
    const std::string& first_name,
    const std::string& last_name,
    const std::string& password
) {
    if (login.empty() || first_name.empty() || last_name.empty() || password.empty()) {
        return std::nullopt;
    }

    auto users = pool_->GetCollection(std::string{kUsersCollection});
    if (users.FindOne(userver::formats::bson::MakeDoc("login", login))) {
        return std::nullopt;
    }

    const auto now = std::chrono::system_clock::now();
    User user;
    user.id = NextId("users");
    user.login = login;
    user.first_name = first_name;
    user.last_name = last_name;
    user.password = password;

    userver::formats::bson::ValueBuilder doc;
    doc["id"] = user.id;
    doc["login"] = user.login;
    doc["first_name"] = user.first_name;
    doc["last_name"] = user.last_name;
    doc["full_name_lc"] = ToLower(user.first_name + " " + user.last_name);
    doc["password_hash"] = HashPassword(password);
    doc["favorite_recipe_ids"] = userver::formats::bson::MakeArray();
    doc["auth_tokens"] = userver::formats::bson::MakeArray();
    doc["created_at"] = now;
    doc["updated_at"] = now;
    users.InsertOne(userver::formats::bson::Document{doc.ExtractValue()});
    return user;
}

std::optional<std::pair<User, std::string>> RecipeStorage::Login(
    const std::string& login,
    const std::string& password
) {
    auto users = pool_->GetCollection(std::string{kUsersCollection});
    const auto doc = users.FindOne(userver::formats::bson::MakeDoc("login", login));
    if (!doc) {
        return std::nullopt;
    }

    const auto stored_hash = (*doc)["password_hash"].As<std::string>("");
    if (stored_hash != HashPassword(password)) {
        return std::nullopt;
    }

    const auto user = UserFromDocument(*doc);
    const auto token = "token-" + std::to_string(user.id) + "-" + std::to_string(NextId("tokens"));
    const auto now = std::chrono::system_clock::now();

    userver::formats::bson::ValueBuilder token_doc;
    token_doc["token"] = token;
    token_doc["issued_at"] = now;

    users.UpdateOne(
        userver::formats::bson::MakeDoc("id", user.id),
        userver::formats::bson::MakeDoc(
            "$push",
            userver::formats::bson::MakeDoc("auth_tokens", token_doc.ExtractValue()),
            "$set",
            userver::formats::bson::MakeDoc("updated_at", now)
        )
    );

    return std::make_pair(user, token);
}

std::optional<User> RecipeStorage::FindUserByLogin(const std::string& login) const {
    auto users = pool_->GetCollection(std::string{kUsersCollection});
    const auto doc = users.FindOne(userver::formats::bson::MakeDoc("login", login));
    if (!doc) {
        return std::nullopt;
    }
    return UserFromDocument(*doc);
}

std::vector<User> RecipeStorage::SearchUsersByMask(const std::string& mask) const {
    auto users = pool_->GetCollection(std::string{kUsersCollection});
    const auto lowered_mask = ToLower(mask);

    std::vector<User> result;
    for (const auto& doc : users.Find(userver::formats::bson::MakeDoc())) {
        const auto full_name = doc["full_name_lc"].As<std::string>("");
        if (full_name.find(lowered_mask) != std::string::npos) {
            result.push_back(UserFromDocument(doc));
        }
    }

    SortUsersById(result);
    return result;
}

std::optional<int> RecipeStorage::ValidateToken(const std::string& token) const {
    auto users = pool_->GetCollection(std::string{kUsersCollection});
    const auto doc = users.FindOne(userver::formats::bson::MakeDoc("auth_tokens.token", token));
    if (!doc) {
        return std::nullopt;
    }
    return (*doc)["id"].As<int>();
}

std::optional<Recipe> RecipeStorage::CreateRecipe(
    int author_id,
    const std::string& title,
    const std::string& description
) {
    if (title.empty() || description.empty() || !FindUserById(author_id)) {
        return std::nullopt;
    }

    const auto now = std::chrono::system_clock::now();
    Recipe recipe;
    recipe.id = NextId("recipes");
    recipe.author_id = author_id;
    recipe.title = title;
    recipe.description = description;

    userver::formats::bson::ValueBuilder doc;
    doc["id"] = recipe.id;
    doc["author_id"] = recipe.author_id;
    doc["title"] = recipe.title;
    doc["title_lc"] = ToLower(recipe.title);
    doc["description"] = recipe.description;
    doc["ingredients"] = userver::formats::bson::MakeArray();
    doc["created_at"] = now;
    doc["updated_at"] = now;

    pool_->GetCollection(std::string{kRecipesCollection}).InsertOne(userver::formats::bson::Document{doc.ExtractValue()});
    return recipe;
}

std::vector<Recipe> RecipeStorage::ListRecipes(const std::optional<std::string>& title_filter) const {
    auto recipes = pool_->GetCollection(std::string{kRecipesCollection});
    const auto lowered_filter = title_filter ? ToLower(*title_filter) : std::string{};

    std::vector<Recipe> result;
    for (const auto& doc : recipes.Find(userver::formats::bson::MakeDoc())) {
        const auto title_lc = doc["title_lc"].As<std::string>("");
        if (!title_filter || title_lc.find(lowered_filter) != std::string::npos) {
            result.push_back(RecipeFromDocument(doc));
        }
    }

    SortRecipesById(result);
    return result;
}

std::optional<Recipe> RecipeStorage::GetRecipe(int recipe_id) const {
    auto recipes = pool_->GetCollection(std::string{kRecipesCollection});
    const auto doc = recipes.FindOne(userver::formats::bson::MakeDoc("id", recipe_id));
    if (!doc) {
        return std::nullopt;
    }
    return RecipeFromDocument(*doc);
}

std::pair<RecipeStorage::AddIngredientStatus, std::optional<Ingredient>> RecipeStorage::AddIngredient(
    int user_id,
    int recipe_id,
    const std::string& name,
    const std::string& amount
) {
    if (name.empty() || amount.empty()) {
        return {AddIngredientStatus::kInvalidInput, std::nullopt};
    }

    const auto recipe = GetRecipe(recipe_id);
    if (!recipe) {
        return {AddIngredientStatus::kRecipeNotFound, std::nullopt};
    }
    if (recipe->author_id != user_id) {
        return {AddIngredientStatus::kForbidden, std::nullopt};
    }

    Ingredient ingredient;
    ingredient.id = NextId("ingredients");
    ingredient.name = name;
    ingredient.amount = amount;

    const auto now = std::chrono::system_clock::now();
    userver::formats::bson::ValueBuilder ingredient_doc;
    ingredient_doc["id"] = ingredient.id;
    ingredient_doc["name"] = ingredient.name;
    ingredient_doc["amount"] = ingredient.amount;
    ingredient_doc["created_at"] = now;

    auto recipes = pool_->GetCollection(std::string{kRecipesCollection});
    recipes.UpdateOne(
        userver::formats::bson::MakeDoc("id", recipe_id, "author_id", user_id),
        userver::formats::bson::MakeDoc(
            "$push",
            userver::formats::bson::MakeDoc("ingredients", ingredient_doc.ExtractValue()),
            "$set",
            userver::formats::bson::MakeDoc("updated_at", now)
        )
    );

    return {AddIngredientStatus::kAdded, ingredient};
}

std::optional<std::vector<Ingredient>> RecipeStorage::GetIngredients(int recipe_id) const {
    const auto recipe = GetRecipe(recipe_id);
    if (!recipe) {
        return std::nullopt;
    }

    auto ingredients = recipe->ingredients;
    SortIngredientsById(ingredients);
    return ingredients;
}

std::vector<Recipe> RecipeStorage::GetRecipesByUser(int user_id) const {
    auto recipes = pool_->GetCollection(std::string{kRecipesCollection});
    std::vector<Recipe> result;

    for (const auto& doc : recipes.Find(userver::formats::bson::MakeDoc("author_id", user_id))) {
        result.push_back(RecipeFromDocument(doc));
    }

    SortRecipesById(result);
    return result;
}

RecipeStorage::FavoriteStatus RecipeStorage::AddFavorite(int user_id, int recipe_id) {
    auto users = pool_->GetCollection(std::string{kUsersCollection});
    auto recipes = pool_->GetCollection(std::string{kRecipesCollection});

    if (!users.FindOne(userver::formats::bson::MakeDoc("id", user_id))) {
        return FavoriteStatus::kUserNotFound;
    }
    if (!recipes.FindOne(userver::formats::bson::MakeDoc("id", recipe_id))) {
        return FavoriteStatus::kRecipeNotFound;
    }

    const auto result = users.UpdateOne(
        userver::formats::bson::MakeDoc(
            "id",
            user_id,
            "favorite_recipe_ids",
            userver::formats::bson::MakeDoc("$ne", recipe_id)
        ),
        userver::formats::bson::MakeDoc(
            "$addToSet",
            userver::formats::bson::MakeDoc("favorite_recipe_ids", recipe_id),
            "$set",
            userver::formats::bson::MakeDoc("updated_at", std::chrono::system_clock::now())
        )
    );

    return result.MatchedCount() > 0 ? FavoriteStatus::kAdded : FavoriteStatus::kAlreadyExists;
}

std::vector<Recipe> RecipeStorage::GetFavorites(int user_id) const {
    std::vector<Recipe> result;
    const auto user_doc = pool_->GetCollection(std::string{kUsersCollection})
                              .FindOne(userver::formats::bson::MakeDoc("id", user_id));
    if (!user_doc) {
        return result;
    }

    std::vector<int> favorite_ids;
    for (const auto& value : (*user_doc)["favorite_recipe_ids"]) {
        favorite_ids.push_back(value.As<int>());
    }
    if (favorite_ids.empty()) {
        return result;
    }

    userver::formats::bson::ValueBuilder in_array(
        userver::formats::bson::ValueBuilder::Type::kArray
    );
    for (const auto favorite_id : favorite_ids) {
        in_array.PushBack(userver::formats::bson::ValueBuilder{favorite_id});
    }

    auto recipes = pool_->GetCollection(std::string{kRecipesCollection});
    for (const auto& doc : recipes.Find(
             userver::formats::bson::MakeDoc(
                 "id",
                 userver::formats::bson::MakeDoc("$in", in_array.ExtractValue())
             )
         )) {
        result.push_back(RecipeFromDocument(doc));
    }

    SortRecipesById(result);
    return result;
}

std::optional<User> RecipeStorage::FindUserById(int user_id) const {
    auto users = pool_->GetCollection(std::string{kUsersCollection});
    const auto doc = users.FindOne(userver::formats::bson::MakeDoc("id", user_id));
    if (!doc) {
        return std::nullopt;
    }
    return UserFromDocument(*doc);
}

int RecipeStorage::NextId(const std::string& sequence_name) {
    auto counters = pool_->GetCollection(std::string{kCountersCollection});
    const auto result = counters.FindAndModify(
        userver::formats::bson::MakeDoc("_id", sequence_name),
        userver::formats::bson::MakeDoc(
            "$inc",
            userver::formats::bson::MakeDoc("seq", 1),
            "$setOnInsert",
            userver::formats::bson::MakeDoc(
                "created_at",
                std::chrono::system_clock::now(),
                "name",
                sequence_name
            )
        ),
        userver::storages::mongo::options::Upsert{},
        userver::storages::mongo::options::ReturnNew{}
    );

    const auto& doc = result.FoundDocument();
    if (!doc) {
        throw std::runtime_error("FindAndModify for counter returned no document");
    }

    return (*doc)["seq"].As<int>();
}

User RecipeStorage::UserFromDocument(const userver::formats::bson::Document& doc) {
    User user;
    user.id = doc["id"].As<int>();
    user.login = doc["login"].As<std::string>();
    user.first_name = doc["first_name"].As<std::string>();
    user.last_name = doc["last_name"].As<std::string>();
    user.password.clear();

    for (const auto& value : doc["favorite_recipe_ids"]) {
        user.favorite_recipe_ids.insert(value.As<int>());
    }
    return user;
}

Recipe RecipeStorage::RecipeFromDocument(const userver::formats::bson::Document& doc) {
    Recipe recipe;
    recipe.id = doc["id"].As<int>();
    recipe.author_id = doc["author_id"].As<int>();
    recipe.title = doc["title"].As<std::string>();
    recipe.description = doc["description"].As<std::string>();

    for (const auto& ingredient_value : doc["ingredients"]) {
        recipe.ingredients.push_back(IngredientFromValue(ingredient_value));
    }

    SortIngredientsById(recipe.ingredients);
    return recipe;
}

Ingredient RecipeStorage::IngredientFromValue(const userver::formats::bson::Value& value) {
    Ingredient ingredient;
    ingredient.id = value["id"].As<int>();
    ingredient.name = value["name"].As<std::string>();
    ingredient.amount = value["amount"].As<std::string>();
    return ingredient;
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
