#include "handlers/auth_handlers.hpp"

#include <userver/formats/common/type.hpp>
#include <userver/formats/json/value_builder.hpp>

#include "domain/json.hpp"
#include "utils/http_utils.hpp"

namespace recipe_service {

std::string RegisterHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&
) const {
    try {
        const auto json = ParseBodyJson(request);
        const auto login = json["login"].As<std::string>("");
        const auto first_name = json["first_name"].As<std::string>("");
        const auto last_name = json["last_name"].As<std::string>("");
        const auto password = json["password"].As<std::string>("");

        if (login.empty() || first_name.empty() || last_name.empty() || password.empty()) {
            return MakeJsonResponse(request, MakeErrorJson("All user fields are required"), HttpStatus::kBadRequest);
        }

        const auto user = GetStorage().RegisterUser(login, first_name, last_name, password);
        if (!user) {
            return MakeJsonResponse(
                request,
                MakeErrorJson("User with this login already exists"),
                HttpStatus::kConflict
            );
        }

        return MakeJsonResponse(request, UserToJson(*user), HttpStatus::kCreated);
    } catch (const std::exception& ex) {
        return MakeJsonResponse(request, MakeErrorJson(ex.what()), HttpStatus::kBadRequest);
    }
}

std::string LoginHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&
) const {
    try {
        const auto json = ParseBodyJson(request);
        const auto login = json["login"].As<std::string>("");
        const auto password = json["password"].As<std::string>("");

        if (login.empty() || password.empty()) {
            return MakeJsonResponse(request, MakeErrorJson("Login and password are required"), HttpStatus::kBadRequest);
        }

        const auto login_result = GetStorage().Login(login, password);
        if (!login_result) {
            return MakeJsonResponse(request, MakeErrorJson("Invalid login or password"), HttpStatus::kUnauthorized);
        }

        userver::formats::json::ValueBuilder builder(userver::formats::common::Type::kObject);
        builder["token"] = login_result->second;
        builder["user"] = UserToJson(login_result->first);
        return MakeJsonResponse(request, builder.ExtractValue(), HttpStatus::kOk);
    } catch (const std::exception& ex) {
        return MakeJsonResponse(request, MakeErrorJson(ex.what()), HttpStatus::kBadRequest);
    }
}

}  // namespace recipe_service
