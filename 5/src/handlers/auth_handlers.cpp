#include "handlers/auth_handlers.hpp"

#include <chrono>
#include <string>

#include <userver/formats/common/type.hpp>
#include <userver/formats/json/value_builder.hpp>

#include "domain/json.hpp"
#include "utils/http_utils.hpp"

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

std::string BuildLoginRateLimitKey(
    const userver::server::http::HttpRequest& request,
    const std::string& login
) {
    const auto forwarded_for = request.GetHeader("X-Forwarded-For");
    if (!login.empty()) {
        return "rate-limit:login:" + login + ":ip:" + forwarded_for;
    }
    return "rate-limit:login:anonymous:ip:" + forwarded_for;
}

}  // namespace


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

        GetCache().Invalidate("users:by-login:" + login);
        GetCache().InvalidatePrefix("users:search:");

        return MakeJsonResponse(request, UserToJson(*user), HttpStatus::kCreated);
    } catch (const std::exception& ex) {
        return MakeExceptionResponse(request, ex);
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

        const auto decision = GetRateLimiter().CheckAndConsume(
            BuildLoginRateLimitKey(request, login),
            GetRateLimiter().GetLoginLimit(),
            GetRateLimiter().GetLoginWindow()
        );
        SetRateLimitHeaders(
            request.GetHttpResponse(),
            decision.limit,
            decision.remaining,
            decision.reset_at
        );

        if (!decision.allowed) {
            return MakeJsonResponse(
                request,
                MakeErrorJson("Too many login attempts. Try again later."),
                HttpStatus::kTooManyRequests
            );
        }

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
        return MakeExceptionResponse(request, ex);
    }
}

}  // namespace recipe_service
