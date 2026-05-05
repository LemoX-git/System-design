#include "handlers/user_handlers.hpp"

#include <string>

#include <userver/formats/common/type.hpp>
#include <userver/formats/json/value_builder.hpp>

#include "domain/json.hpp"
#include "utils/http_utils.hpp"
#include "utils/string_utils.hpp"

namespace recipe_service {
namespace {

std::string MakeExceptionResponse(
    const userver::server::http::HttpRequest& request,
    const std::exception& ex
) {
    if (dynamic_cast<const ClientError*>(&ex) != nullptr) {
        return MakeJsonResponse(request, MakeErrorJson(ex.what()), HttpStatus::kBadRequest);
    }
    return MakeJsonResponse(request, MakeErrorJson("Internal server error"), HttpStatus::kInternalServerError);
}

}  // namespace


std::string UserByLoginHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&
) const {
    try {
        const auto login = std::string{request.GetArg("login")};
        if (login.empty()) {
            return MakeJsonResponse(request, MakeErrorJson("Query parameter 'login' is required"), HttpStatus::kBadRequest);
        }

        const auto cache_key = "users:by-login:" + login;
        if (const auto cached = GetCache().Get(cache_key)) {
            request.GetHttpResponse().SetHeader(std::string{"X-Cache"}, std::string{"HIT"});
            return MakeRawJsonResponse(request, *cached);
        }

        const auto user = GetStorage().FindUserByLogin(login);
        if (!user) {
            return MakeJsonResponse(request, MakeErrorJson("User not found"), HttpStatus::kNotFound);
        }

        const auto response = MakeJsonResponse(request, UserToJson(*user));
        request.GetHttpResponse().SetHeader(std::string{"X-Cache"}, std::string{"MISS"});
        GetCache().Put(cache_key, response, GetCache().GetUsersTtl());
        return response;
    } catch (const std::exception& ex) {
        return MakeExceptionResponse(request, ex);
    }
}

std::string UserSearchHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&
) const {
    try {
        const auto mask = std::string{request.GetArg("mask")};
        if (mask.empty()) {
            return MakeJsonResponse(request, MakeErrorJson("Query parameter 'mask' is required"), HttpStatus::kBadRequest);
        }

        const auto cache_key = "users:search:" + ToLower(mask);
        if (const auto cached = GetCache().Get(cache_key)) {
            request.GetHttpResponse().SetHeader(std::string{"X-Cache"}, std::string{"HIT"});
            return MakeRawJsonResponse(request, *cached);
        }

        const auto users = GetStorage().SearchUsersByMask(mask);
        userver::formats::json::ValueBuilder array(userver::formats::common::Type::kArray);
        for (const auto& user : users) {
            array.PushBack(UserToJson(user));
        }

        const auto response = MakeJsonResponse(request, array.ExtractValue());
        request.GetHttpResponse().SetHeader(std::string{"X-Cache"}, std::string{"MISS"});
        GetCache().Put(cache_key, response, GetCache().GetUsersTtl());
        return response;
    } catch (const std::exception& ex) {
        return MakeExceptionResponse(request, ex);
    }
}

}  // namespace recipe_service
