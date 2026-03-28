#include "utils/string_utils.hpp"

#include <algorithm>
#include <cctype>

namespace recipe_service {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

}  // namespace recipe_service
