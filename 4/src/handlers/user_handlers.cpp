#include "handlers/user_handlers.hpp"

#include <userver/formats/common/type.hpp>
#include <userver/formats/json/value_builder.hpp>

#include "domain/json.hpp"
#include "utils/http_utils.hpp"

namespace recipe_service {

std::string UserByLoginHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&
) const {
    const auto& login = request.GetArg("login");
    if (login.empty()) {
        return MakeJsonResponse(request, MakeErrorJson("Query parameter 'login' is required"), HttpStatus::kBadRequest);
    }

    const auto user = GetStorage().FindUserByLogin(login);
    if (!user) {
        return MakeJsonResponse(request, MakeErrorJson("User not found"), HttpStatus::kNotFound);
    }

    return MakeJsonResponse(request, UserToJson(*user));
}

std::string UserSearchHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&
) const {
    const auto& mask = request.GetArg("mask");
    if (mask.empty()) {
        return MakeJsonResponse(request, MakeErrorJson("Query parameter 'mask' is required"), HttpStatus::kBadRequest);
    }

    const auto users = GetStorage().SearchUsersByMask(mask);
    userver::formats::json::ValueBuilder array(userver::formats::common::Type::kArray);
    for (const auto& user : users) {
        array.PushBack(UserToJson(user));
    }
    return MakeJsonResponse(request, array.ExtractValue());
}

}  // namespace recipe_service
