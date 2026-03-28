#include "utils/http_utils.hpp"

#include <stdexcept>

#include <fmt/format.h>

#include <userver/formats/common/type.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_response.hpp>

namespace recipe_service {

userver::formats::json::Value MakeErrorJson(const std::string& message) {
    userver::formats::json::ValueBuilder builder(userver::formats::common::Type::kObject);
    builder["error"] = message;
    return builder.ExtractValue();
}

std::string MakeJsonResponse(
    const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value& value,
    HttpStatus status
) {
    request.SetResponseStatus(status);
    request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);
    return userver::formats::json::ToString(value);
}

void SetJsonResponseBody(
    userver::server::http::HttpResponse& response,
    const userver::formats::json::Value& value,
    HttpStatus status
) {
    response.SetStatus(status);
    response.SetContentType(userver::http::content_type::kApplicationJson);
    response.SetData(userver::formats::json::ToString(value));
}

int ParseId(const std::string& raw, const std::string& field_name) {
    try {
        return std::stoi(raw);
    } catch (const std::exception&) {
        throw std::runtime_error(fmt::format("Invalid '{}' path parameter", field_name));
    }
}

userver::formats::json::Value ParseBodyJson(const userver::server::http::HttpRequest& request) {
    if (request.RequestBody().empty()) {
        return userver::formats::json::FromString("{}");
    }
    return userver::formats::json::FromString(request.RequestBody());
}

int GetAuthorizedUserId(userver::server::request::RequestContext& context) {
    auto* user_id = context.GetDataOptional<int>(kUserIdContextKey);
    if (!user_id) {
        throw std::runtime_error("Authorized user id was not found in request context");
    }
    return *user_id;
}

}  // namespace recipe_service
