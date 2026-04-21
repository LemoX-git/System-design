#pragma once

#include <string>
#include <unordered_set>
#include <vector>

namespace recipe_service {

struct Ingredient {
    int id{};
    std::string name;
    std::string amount;
};

struct Recipe {
    int id{};
    int author_id{};
    std::string title;
    std::string description;
    std::vector<Ingredient> ingredients;
};

struct User {
    int id{};
    std::string login;
    std::string first_name;
    std::string last_name;
    std::string password;
    std::unordered_set<int> favorite_recipe_ids;
};

}  // namespace recipe_service
