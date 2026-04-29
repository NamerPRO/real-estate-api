#include "./rate_limiter_component.hpp"

namespace components {

RateLimiterComponent::RateLimiterComponent(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : ComponentBase(config, context) {}

bool RateLimiterComponent::CheckRateLimit(
          const std::string &key) const {
  auto now = std::chrono::steady_clock::now();

  auto &entry = data_[key];

  if (now - entry.ts > std::chrono::seconds(timeSec)) {
    entry.count = 0;
    entry.ts = now;
  }

  if (entry.count >= limit) {
    return false;
  }

  ++entry.count;
  return true;
}
} // namespace components