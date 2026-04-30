#include "./rate_limiter_component.hpp"
#include <chrono>
#include <userver/logging/log.hpp>

namespace components {

RateLimiterComponent::RateLimiterComponent(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : ComponentBase(config, context) {}

int RateLimiterComponent::GetRemaining(
  const std::string &key
) const {
  auto &entry = data_[key];
  return std::max(entry.tokens, 0.0);
}

bool RateLimiterComponent::CheckRateLimit(const std::string &key) const {
  const auto now = std::chrono::steady_clock::now();

  std::lock_guard<std::mutex> lock(mutex_);

  auto &entry = data_[key];

  if (entry.ts.time_since_epoch().count() == 0) {
    entry.ts = now;
    entry.tokens = limit;
  }

  const double elapsed = std::chrono::duration<double>(now - entry.ts).count();

  const double rate = limit / timeSec;

  entry.tokens = std::min(limit, entry.tokens + elapsed * rate);

  entry.ts = now;

  if (entry.tokens < 1.0) {
    return false;
  }

  entry.tokens -= 1.0;
  return true;
}
} // namespace components