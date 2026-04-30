#pragma once

#include <chrono>
#include <jwt-cpp/jwt.h>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include "../models/auth.hpp"

namespace components {

class AuthComponent : public userver::components::ComponentBase {
public:
  static constexpr std::string_view kName = "auth-component";

  explicit AuthComponent(const userver::components::ComponentConfig &config,
                         const userver::components::ComponentContext &context);

  static userver::yaml_config::Schema GetStaticConfigSchema();

  std::string GenerateToken(int64_t user_id, const std::string &login);

  std::optional<std::string> GetActiveToken(int64_t user_id);

  std::optional<models::dto::AuthTokenData> ValidateToken(const std::string &token);

  static std::optional<std::string>
  ExtractTokenFromHeader(const userver::server::http::HttpRequest &request);

  static std::string HashPassword(const std::string &password);
  static bool VerifyPassword(const std::string &password,
                             const std::string &hash);

private:
  std::string secret_key_;
  int token_lifetime_seconds_;

  struct ActiveToken {
    std::string token;
    std::chrono::system_clock::time_point expiry;
  };

  std::unordered_map<int64_t, ActiveToken> active_tokens_;
  std::mutex tokens_mutex_;

  void CleanupExpiredTokens();
};

} // namespace components