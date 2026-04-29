#pragma once

#include <unordered_map>
#include <chrono>
#include <string>
#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>

namespace components {
class RateLimiterComponent : public userver::components::ComponentBase {
public:
  static constexpr int limit = 100;
  static constexpr int timeSec = 60;

  static constexpr std::string_view kName = "rate-limit-component";

  explicit RateLimiterComponent(
      const userver::components::ComponentConfig &config,
      const userver::components::ComponentContext &context);

  bool CheckRateLimit(const std::string &key) const;

private:
  struct Entry {
    int count{0};
    std::chrono::steady_clock::time_point ts{};
  };

  mutable std::unordered_map<std::string, Entry> data_;
};
} // namespace components