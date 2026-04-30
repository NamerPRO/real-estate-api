#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>

namespace components {
class RateLimiterComponent : public userver::components::ComponentBase {
public:
  static constexpr double limit = 100;
  static constexpr int timeSec = 60;

  static constexpr std::string_view kName = "rate-limit-component";

  explicit RateLimiterComponent(
      const userver::components::ComponentConfig &config,
      const userver::components::ComponentContext &context);

  bool CheckRateLimit(const std::string &key) const;

  int GetRemaining(const std::string &key) const;

private:
  struct Entry {
    double tokens{limit};
    std::chrono::steady_clock::time_point ts{};
  };

  mutable std::mutex mutex_;
  mutable std::unordered_map<std::string, Entry> data_;
};
} // namespace components