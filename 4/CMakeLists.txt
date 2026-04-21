cmake_minimum_required(VERSION 3.16)
project(recipe_service LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(userver COMPONENTS core mongo REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/domain/json.cpp
    src/utils/string_utils.cpp
    src/utils/http_utils.cpp
    src/storage/recipe_storage.cpp
    src/middlewares/auth_middleware.cpp
    src/handlers/auth_handlers.cpp
    src/handlers/user_handlers.cpp
    src/handlers/recipe_handlers.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE userver::core userver::mongo)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

enable_testing()

userver_testsuite_add_simple(
    SERVICE_TARGET ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
)