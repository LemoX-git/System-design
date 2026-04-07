#include "storage/recipe_storage.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <openssl/sha.h>
#include <libpq-fe.h>

#include <fmt/format.h>
#include <userver/components/component_context.hpp>
#include <userver/components/component_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include "utils/string_utils.hpp"

namespace recipe_service {
namespace {

using ResultPtr = std::unique_ptr<PGresult, decltype(&PQclear)>;

std::string Sha256Hex(const std::string& value) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(
        reinterpret_cast<const unsigned char*>(value.data()),
        value.size(),
        hash
    );

    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (unsigned char byte : hash) {
        out << std::setw(2) << static_cast<int>(byte);
    }
    return out.str();
}

std::string GenerateToken() {
    static thread_local std::mt19937_64 generator{std::random_device{}()};
    std::uniform_int_distribution<unsigned long long> distribution;

    std::ostringstream out;
    out << std::hex << distribution(generator) << distribution(generator);
    return out.str();
}

void EnsureResultOk(const PGresult& result, const std::string& action) {
    const auto status = PQresultStatus(&result);
    if (
        status != PGRES_COMMAND_OK &&
        status != PGRES_TUPLES_OK
    ) {
        throw std::runtime_error(fmt::format(
            "Failed to {}: {}",
            action,
            PQresultErrorMessage(&result)
        ));
    }
}

ResultPtr ExecCommand(
    PGconn* connection,
    const std::string& action,
    const std::string& query,
    const std::vector<std::string>& params = {}
) {
    ResultPtr result(nullptr, &PQclear);

    if (params.empty()) {
        result.reset(PQexec(connection, query.c_str()));
    } else {
        std::vector<const char*> param_values;
        param_values.reserve(params.size());
        for (const auto& param : params) {
            param_values.push_back(param.c_str());
        }

        result.reset(
            PQexecParams(
                connection,
                query.c_str(),
                static_cast<int>(param_values.size()),
                nullptr,
                param_values.data(),
                nullptr,
                nullptr,
                0
            )
        );
    }

    if (!result) {
        throw std::runtime_error(fmt::format("Failed to {}: empty PostgreSQL result", action));
    }

    EnsureResultOk(*result, action);
    return result;
}

std::string GetString(const PGresult& result, int row, int column) {
    return std::string{PQgetvalue(&result, row, column)};
}

int GetInt(const PGresult& result, int row, int column) {
    return std::stoi(GetString(result, row, column));
}

User ReadUser(const PGresult& result, int row, bool with_password_hash = true) {
    User user;
    user.id = GetInt(result, row, 0);
    user.login = GetString(result, row, 1);
    user.first_name = GetString(result, row, 2);
    user.last_name = GetString(result, row, 3);
    if (with_password_hash) {
        user.password_hash = GetString(result, row, 4);
    }
    return user;
}

Recipe ReadRecipe(
    const PGresult& result,
    int row,
    int id_column = 0,
    int author_column = 1,
    int title_column = 2,
    int description_column = 3,
    int count_column = 4
) {
    Recipe recipe;
    recipe.id = GetInt(result, row, id_column);
    recipe.author_id = GetInt(result, row, author_column);
    recipe.title = GetString(result, row, title_column);
    recipe.description = GetString(result, row, description_column);
    recipe.ingredients_count = GetInt(result, row, count_column);
    return recipe;
}

Ingredient ReadIngredient(const PGresult& result, int row) {
    Ingredient ingredient;
    ingredient.id = GetInt(result, row, 0);
    ingredient.recipe_id = GetInt(result, row, 1);
    ingredient.name = GetString(result, row, 2);
    ingredient.amount = GetString(result, row, 3);
    return ingredient;
}

}  // namespace

RecipeStorage::RecipeStorage(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
) : userver::components::ComponentBase(config, context),
    dbconnection_(config["dbconnection"].As<std::string>()),
    schema_path_(config["schema-path"].As<std::string>("schema.sql")) {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);
    ConnectLocked();
    InitializeSchemaLocked();
}

RecipeStorage::~RecipeStorage() {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);
    if (connection_) {
        PQfinish(connection_);
        connection_ = nullptr;
    }
}

userver::yaml_config::Schema RecipeStorage::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(R"(
        type: object
        description: Storage component backed by PostgreSQL for recipe service.
        additionalProperties: false
        properties:
            dbconnection:
                type: string
                description: PostgreSQL connection string in libpq format.
                defaultDescription: libpq connection string.
            schema-path:
                type: string
                description: Path to schema.sql used on startup.
                default: schema.sql
    )");
}

void RecipeStorage::ConnectLocked() {
    if (connection_) {
        PQfinish(connection_);
        connection_ = nullptr;
    }

    connection_ = PQconnectdb(dbconnection_.c_str());
    if (!connection_ || PQstatus(connection_) != CONNECTION_OK) {
        const std::string error = connection_ ? PQerrorMessage(connection_) : "Unknown PostgreSQL connection error";
        throw std::runtime_error(fmt::format("Could not connect to PostgreSQL: {}", error));
    }
}

void RecipeStorage::InitializeSchemaLocked() {
    std::vector<std::string> candidate_paths{schema_path_, "../" + schema_path_, "/app/" + schema_path_};

    std::ifstream schema_file;
    std::string resolved_path;
    for (const auto& path : candidate_paths) {
        if (std::filesystem::exists(path)) {
            schema_file.open(path);
            if (schema_file) {
                resolved_path = path;
                break;
            }
        }
    }

    if (!schema_file) {
        throw std::runtime_error(fmt::format("Could not open schema file '{}'", schema_path_));
    }

    const std::string schema_sql{
        std::istreambuf_iterator<char>{schema_file},
        std::istreambuf_iterator<char>{}
    };

    auto result = ExecCommand(connection_, "initialize database schema from " + resolved_path, schema_sql);
    (void)result;
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

    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    auto result = ExecCommand(
        connection_,
        "register user",
        R"SQL(
            INSERT INTO users (login, first_name, last_name, password_hash)
            VALUES ($1, $2, $3, $4)
            ON CONFLICT (login) DO NOTHING
            RETURNING id, login, first_name, last_name, password_hash
        )SQL",
        {login, first_name, last_name, Sha256Hex(password)}
    );

    if (PQntuples(result.get()) == 0) {
        return std::nullopt;
    }
    return ReadUser(*result, 0);
}

std::optional<std::pair<User, std::string>> RecipeStorage::Login(
    const std::string& login,
    const std::string& password
) {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    auto user_result = ExecCommand(
        connection_,
        "load user by login",
        R"SQL(
            SELECT id, login, first_name, last_name, password_hash
            FROM users
            WHERE login = $1
        )SQL",
        {login}
    );

    if (PQntuples(user_result.get()) == 0) {
        return std::nullopt;
    }

    const auto user = ReadUser(*user_result, 0);
    if (user.password_hash != Sha256Hex(password)) {
        return std::nullopt;
    }

    const auto token = GenerateToken();
    auto token_result = ExecCommand(
        connection_,
        "store auth token",
        R"SQL(
            INSERT INTO auth_tokens (token, user_id)
            VALUES ($1, $2)
        )SQL",
        {token, std::to_string(user.id)}
    );
    (void)token_result;

    User public_user = user;
    public_user.password_hash.clear();
    return std::make_pair(public_user, token);
}

std::optional<User> RecipeStorage::FindUserByLogin(const std::string& login) const {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    auto result = ExecCommand(
        connection_,
        "find user by login",
        R"SQL(
            SELECT id, login, first_name, last_name
            FROM users
            WHERE login = $1
        )SQL",
        {login}
    );

    if (PQntuples(result.get()) == 0) {
        return std::nullopt;
    }

    return ReadUser(*result, 0, false);
}

std::vector<User> RecipeStorage::SearchUsersByMask(const std::string& mask) const {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    auto result = ExecCommand(
        connection_,
        "search users by mask",
        R"SQL(
            SELECT id, login, first_name, last_name
            FROM users
            WHERE lower(first_name || ' ' || last_name) LIKE $1
            ORDER BY id
        )SQL",
        {"%" + ToLower(mask) + "%"}
    );

    std::vector<User> users;
    users.reserve(PQntuples(result.get()));
    for (int row = 0; row < PQntuples(result.get()); ++row) {
        users.push_back(ReadUser(*result, row, false));
    }
    return users;
}

std::optional<int> RecipeStorage::ValidateToken(const std::string& token) const {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    auto result = ExecCommand(
        connection_,
        "validate auth token",
        R"SQL(
            SELECT user_id
            FROM auth_tokens
            WHERE token = $1
              AND (expires_at IS NULL OR expires_at > now())
        )SQL",
        {token}
    );

    if (PQntuples(result.get()) == 0) {
        return std::nullopt;
    }

    return GetInt(*result, 0, 0);
}

std::optional<Recipe> RecipeStorage::CreateRecipe(
    int author_id,
    const std::string& title,
    const std::string& description
) {
    if (title.empty() || description.empty()) {
        return std::nullopt;
    }

    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    auto result = ExecCommand(
        connection_,
        "create recipe",
        R"SQL(
            INSERT INTO recipes (author_id, title, description)
            VALUES ($1, $2, $3)
            RETURNING id, author_id, title, description, 0
        )SQL",
        {std::to_string(author_id), title, description}
    );

    if (PQntuples(result.get()) == 0) {
        return std::nullopt;
    }

    return ReadRecipe(*result, 0);
}

std::vector<Recipe> RecipeStorage::ListRecipes(const std::optional<std::string>& title_filter) const {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    const std::string query = title_filter
        ? R"SQL(
            SELECT r.id,
                   r.author_id,
                   r.title,
                   r.description,
                   COUNT(i.id)::int AS ingredients_count
            FROM recipes r
            LEFT JOIN ingredients i ON i.recipe_id = r.id
            WHERE lower(r.title) LIKE $1
            GROUP BY r.id, r.author_id, r.title, r.description
            ORDER BY r.id
        )SQL"
        : R"SQL(
            SELECT r.id,
                   r.author_id,
                   r.title,
                   r.description,
                   COUNT(i.id)::int AS ingredients_count
            FROM recipes r
            LEFT JOIN ingredients i ON i.recipe_id = r.id
            GROUP BY r.id, r.author_id, r.title, r.description
            ORDER BY r.id
        )SQL";

    const std::vector<std::string> params = title_filter
        ? std::vector<std::string>{"%" + ToLower(*title_filter) + "%"}
        : std::vector<std::string>{};

    auto result = ExecCommand(connection_, "list recipes", query, params);

    std::vector<Recipe> recipes;
    recipes.reserve(PQntuples(result.get()));
    for (int row = 0; row < PQntuples(result.get()); ++row) {
        recipes.push_back(ReadRecipe(*result, row));
    }
    return recipes;
}

std::optional<Recipe> RecipeStorage::GetRecipe(int recipe_id) const {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    auto result = ExecCommand(
        connection_,
        "get recipe",
        R"SQL(
            SELECT r.id,
                   r.author_id,
                   r.title,
                   r.description,
                   COUNT(i.id)::int AS ingredients_count
            FROM recipes r
            LEFT JOIN ingredients i ON i.recipe_id = r.id
            WHERE r.id = $1
            GROUP BY r.id, r.author_id, r.title, r.description
        )SQL",
        {std::to_string(recipe_id)}
    );

    if (PQntuples(result.get()) == 0) {
        return std::nullopt;
    }

    return ReadRecipe(*result, 0);
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

    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    auto recipe_result = ExecCommand(
        connection_,
        "load recipe author",
        R"SQL(
            SELECT author_id
            FROM recipes
            WHERE id = $1
        )SQL",
        {std::to_string(recipe_id)}
    );

    if (PQntuples(recipe_result.get()) == 0) {
        return {AddIngredientStatus::kRecipeNotFound, std::nullopt};
    }

    const int author_id = GetInt(*recipe_result, 0, 0);
    if (author_id != user_id) {
        return {AddIngredientStatus::kForbidden, std::nullopt};
    }

    auto insert_result = ExecCommand(
        connection_,
        "add ingredient",
        R"SQL(
            INSERT INTO ingredients (recipe_id, name, amount)
            VALUES ($1, $2, $3)
            RETURNING id, recipe_id, name, amount
        )SQL",
        {std::to_string(recipe_id), name, amount}
    );

    return {AddIngredientStatus::kAdded, ReadIngredient(*insert_result, 0)};
}

std::optional<std::vector<Ingredient>> RecipeStorage::GetIngredients(int recipe_id) const {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    auto recipe_result = ExecCommand(
        connection_,
        "check recipe existence",
        R"SQL(
            SELECT 1
            FROM recipes
            WHERE id = $1
        )SQL",
        {std::to_string(recipe_id)}
    );

    if (PQntuples(recipe_result.get()) == 0) {
        return std::nullopt;
    }

    auto result = ExecCommand(
        connection_,
        "get ingredients",
        R"SQL(
            SELECT id, recipe_id, name, amount
            FROM ingredients
            WHERE recipe_id = $1
            ORDER BY id
        )SQL",
        {std::to_string(recipe_id)}
    );

    std::vector<Ingredient> ingredients;
    ingredients.reserve(PQntuples(result.get()));
    for (int row = 0; row < PQntuples(result.get()); ++row) {
        ingredients.push_back(ReadIngredient(*result, row));
    }
    return ingredients;
}

std::vector<Recipe> RecipeStorage::GetRecipesByUser(int user_id) const {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    auto result = ExecCommand(
        connection_,
        "get recipes by user",
        R"SQL(
            SELECT r.id,
                   r.author_id,
                   r.title,
                   r.description,
                   COUNT(i.id)::int AS ingredients_count
            FROM recipes r
            LEFT JOIN ingredients i ON i.recipe_id = r.id
            WHERE r.author_id = $1
            GROUP BY r.id, r.author_id, r.title, r.description
            ORDER BY r.id
        )SQL",
        {std::to_string(user_id)}
    );

    std::vector<Recipe> recipes;
    recipes.reserve(PQntuples(result.get()));
    for (int row = 0; row < PQntuples(result.get()); ++row) {
        recipes.push_back(ReadRecipe(*result, row));
    }
    return recipes;
}

RecipeStorage::FavoriteStatus RecipeStorage::AddFavorite(int user_id, int recipe_id) {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    auto user_result = ExecCommand(
        connection_,
        "check user existence",
        R"SQL(
            SELECT 1
            FROM users
            WHERE id = $1
        )SQL",
        {std::to_string(user_id)}
    );
    if (PQntuples(user_result.get()) == 0) {
        return FavoriteStatus::kUserNotFound;
    }

    auto recipe_result = ExecCommand(
        connection_,
        "check recipe existence for favorites",
        R"SQL(
            SELECT 1
            FROM recipes
            WHERE id = $1
        )SQL",
        {std::to_string(recipe_id)}
    );
    if (PQntuples(recipe_result.get()) == 0) {
        return FavoriteStatus::kRecipeNotFound;
    }

    auto insert_result = ExecCommand(
        connection_,
        "insert favorite recipe",
        R"SQL(
            INSERT INTO favorite_recipes (user_id, recipe_id)
            VALUES ($1, $2)
            ON CONFLICT DO NOTHING
            RETURNING user_id
        )SQL",
        {std::to_string(user_id), std::to_string(recipe_id)}
    );

    if (PQntuples(insert_result.get()) == 0) {
        return FavoriteStatus::kAlreadyExists;
    }

    return FavoriteStatus::kAdded;
}

std::vector<Recipe> RecipeStorage::GetFavorites(int user_id) const {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    auto result = ExecCommand(
        connection_,
        "get favorite recipes",
        R"SQL(
            SELECT r.id,
                   r.author_id,
                   r.title,
                   r.description,
                   COUNT(i.id)::int AS ingredients_count
            FROM favorite_recipes f
            JOIN recipes r ON r.id = f.recipe_id
            LEFT JOIN ingredients i ON i.recipe_id = r.id
            WHERE f.user_id = $1
            GROUP BY r.id, r.author_id, r.title, r.description
            ORDER BY r.id
        )SQL",
        {std::to_string(user_id)}
    );

    std::vector<Recipe> recipes;
    recipes.reserve(PQntuples(result.get()));
    for (int row = 0; row < PQntuples(result.get()); ++row) {
        recipes.push_back(ReadRecipe(*result, row));
    }
    return recipes;
}

}  // namespace recipe_service
