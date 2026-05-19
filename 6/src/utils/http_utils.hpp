#pragma once

#include <string>
#include <string_view>
#include <stdexcept>

#include <userver/formats/json/value.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/server/request/request_context.hpp>

namespace userver::server::http {
class HttpRequest;
class HttpResponse;
}  // namespace userver::server::http

namespace recipe_service {

using HttpStatus = userver::server::http::HttpStatus;
inline constexpr std::string_view kUserIdContextKey = "user_id";

class ClientError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

userver::formats::json::Value MakeErrorJson(const std::string& message);
std::string MakeJsonResponse(
    const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value& value,
    HttpStatus status = HttpStatus::kOk
);
void SetJsonResponseBody(
    userver::server::http::HttpResponse& response,
    const userver::formats::json::Value& value,
    HttpStatus status
);
int ParseId(const std::string& raw, const std::string& field_name);
userver::formats::json::Value ParseBodyJson(const userver::server::http::HttpRequest& request);
int GetAuthorizedUserId(userver::server::request::RequestContext& context);

}  // namespace recipe_service
