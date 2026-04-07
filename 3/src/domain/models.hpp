#pragma once

#include <string>
#include <vector>

namespace recipe_service {

struct Ingredient {
    int id{};
    int recipe_id{};
    std::string name;
    std::string amount;
};

struct Recipe {
    int id{};
    int author_id{};
    std::string title;
    std::string description;
    int ingredients_count{};
    std::vector<Ingredient> ingredients;
};

struct User {
    int id{};
    std::string login;
    std::string first_name;
    std::string last_name;
    std::string password_hash;
};

}  // namespace recipe_service
