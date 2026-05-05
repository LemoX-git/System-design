#include "components/rate_limiter.hpp"

#include <vector>

#include <userver/components/component_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace recipe_service {

RateLimiter::RateLimiter(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
) : userver::components::ComponentBase(config, context),
    login_limit_(config["login-limit"].As<int>(5)),
    login_window_(std::chrono::seconds{config["login-window-seconds"].As<int>(60)}) {
}

userver::yaml_config::Schema RateLimiter::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(R"(
        type: object
        description: Fixed-window rate limiter configuration.
        additionalProperties: false
        properties:
            login-limit:
                type: integer
                description: Maximum number of login attempts per window.
                default: 5
            login-window-seconds:
                type: integer
                description: Window size for login rate limiting in seconds.
                default: 60
    )");
}

RateLimiter::Decision RateLimiter::CheckAndConsume(
    const std::string& bucket,
    int limit,
    std::chrono::seconds window
) {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    const auto now = std::chrono::system_clock::now();
    auto& counter = counters_[bucket];
    if (counter.reset_at.time_since_epoch().count() == 0 || now >= counter.reset_at) {
        counter.count = 0;
        counter.reset_at = now + window;
    }

    Decision decision;
    decision.limit = limit;
    decision.reset_at = counter.reset_at;

    if (counter.count >= limit) {
        decision.allowed = false;
        decision.remaining = 0;
        return decision;
    }

    ++counter.count;
    decision.allowed = true;
    decision.remaining = limit - counter.count;
    return decision;
}

int RateLimiter::GetLoginLimit() const {
    return login_limit_;
}

std::chrono::seconds RateLimiter::GetLoginWindow() const {
    return login_window_;
}

}  // namespace recipe_service
