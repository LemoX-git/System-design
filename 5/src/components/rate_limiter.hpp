#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <unordered_map>

#include <userver/components/component_base.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/yaml_config/schema.hpp>

namespace recipe_service {

class RateLimiter final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "rate-limiter";

    struct Decision {
        bool allowed{true};
        int limit{0};
        int remaining{0};
        std::chrono::system_clock::time_point reset_at{};
    };

    RateLimiter(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

    static userver::yaml_config::Schema GetStaticConfigSchema();

    Decision CheckAndConsume(
        const std::string& bucket,
        int limit,
        std::chrono::seconds window
    );

    int GetLoginLimit() const;
    std::chrono::seconds GetLoginWindow() const;

private:
    struct WindowCounter {
        int count{0};
        std::chrono::system_clock::time_point reset_at{};
    };

    mutable userver::engine::Mutex mutex_;
    std::unordered_map<std::string, WindowCounter> counters_;

    int login_limit_;
    std::chrono::seconds login_window_;
};

}  // namespace recipe_service
