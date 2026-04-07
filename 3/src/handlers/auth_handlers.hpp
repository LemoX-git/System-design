#pragma once

#include <string>
#include <string_view>

#include "handlers/base_handler.hpp"

namespace recipe_service {

class RegisterHandler final : public StorageAwareHandler {
public:
    static constexpr std::string_view kName = "handler-register";

    using StorageAwareHandler::StorageAwareHandler;

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context
    ) const override;
};

class LoginHandler final : public StorageAwareHandler {
public:
    static constexpr std::string_view kName = "handler-login";

    using StorageAwareHandler::StorageAwareHandler;

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context
    ) const override;
};

}  // namespace recipe_service
