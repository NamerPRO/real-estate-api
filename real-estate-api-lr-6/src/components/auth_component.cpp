#include "auth_component.hpp"
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>
#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/yaml_config/schema.hpp>

namespace components {

userver::yaml_config::Schema AuthComponent::GetStaticConfigSchema() {
  userver::yaml_config::Schema schema;

  schema.type = userver::yaml_config::FieldType::kObject;
  schema.additional_properties = false;
  schema.required = std::unordered_set<std::string>{"secret_key"};

  schema.properties =
      std::unordered_map<std::string, userver::yaml_config::SchemaPtr>{};

  {
    userver::yaml_config::Schema s;
    s.type = userver::yaml_config::FieldType::kString;
    s.description = "Secret key used for JWT signing";
    schema.properties->emplace(std::string("secret_key"),
                               userver::yaml_config::SchemaPtr(std::move(s)));
  }

  {
    userver::yaml_config::Schema s;
    s.type = userver::yaml_config::FieldType::kInteger;
    s.description = "Token lifetime in seconds";
    schema.properties->emplace(std::string("token_lifetime"),
                               userver::yaml_config::SchemaPtr(std::move(s)));
  }

  return schema;
}

AuthComponent::AuthComponent(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : ComponentBase(config, context),
      secret_key_(config["secret_key"].As<std::string>("my-secret-key")),
      token_lifetime_seconds_(config["token_lifetime"].As<int>(3600)) {}

void AuthComponent::CleanupExpiredTokens() {
  auto now = std::chrono::system_clock::now();
  for (auto it = active_tokens_.begin(); it != active_tokens_.end();) {
    if (now > it->second.expiry) {
      it = active_tokens_.erase(it);
    } else {
      ++it;
    }
  }
}

std::optional<std::string> AuthComponent::GetActiveToken(int64_t user_id) {
  std::lock_guard<std::mutex> lock(tokens_mutex_);

  CleanupExpiredTokens();

  auto it = active_tokens_.find(user_id);
  if (it != active_tokens_.end()) {
    LOG_INFO() << "Found active token for user_id=" << user_id;
    return it->second.token;
  }

  return std::nullopt;
}

std::string AuthComponent::GenerateToken(int64_t user_id,
                                         const std::string &login) {
  auto now = std::chrono::system_clock::now();
  auto expiry = now + std::chrono::seconds(token_lifetime_seconds_);

  auto token =
      ::jwt::create()
          .set_issuer("real-estate-api")
          .set_type("JWT")
          .set_id(std::to_string(user_id))
          .set_issued_at(now)
          .set_expires_at(expiry)
          .set_payload_claim("user_id", ::jwt::claim(std::to_string(user_id)))
          .set_payload_claim("login", ::jwt::claim(login))
          .sign(::jwt::algorithm::hs256{secret_key_});

  {
    std::lock_guard<std::mutex> lock(tokens_mutex_);
    CleanupExpiredTokens();
    active_tokens_[user_id] = {token, expiry};
  }

  LOG_INFO() << "Generated new JWT token for user_id=" << user_id;
  return token;
}

std::optional<models::dto::AuthTokenData>
AuthComponent::ValidateToken(const std::string &token) {
  try {
    auto decoded = ::jwt::decode(token);

    auto verifier = ::jwt::verify()
                        .allow_algorithm(::jwt::algorithm::hs256{secret_key_})
                        .with_issuer("real-estate-api");

    verifier.verify(decoded);

    auto exp = decoded.get_expires_at();
    if (std::chrono::system_clock::now() > exp) {
      return std::nullopt;
    }

    int64_t user_id =
        std::stoll(decoded.get_payload_claim("user_id").as_string());
    std::string login = decoded.get_payload_claim("login").as_string();

    return models::dto::AuthTokenData{user_id, login, true};
  } catch (const std::exception &e) {
    LOG_WARNING() << "Token validation error: " << e.what();
    return std::nullopt;
  }
}

std::optional<std::string> AuthComponent::ExtractTokenFromHeader(
    const userver::server::http::HttpRequest &request) {

  if (!request.HasHeader("Authorization")) {
    return std::nullopt;
  }

  std::string auth_value = request.GetHeader("Authorization");

  if (auth_value.size() < 7 || auth_value.substr(0, 7) != "Bearer ") {
    return std::nullopt;
  }

  return auth_value.substr(7);
}

std::string AuthComponent::HashPassword(const std::string &password) {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  unsigned int hash_len = 0;

  if (1 != EVP_Digest(password.c_str(), password.size(), hash, &hash_len,
                      EVP_sha256(), nullptr)) {
    throw std::runtime_error("Failed to compute hash");
  }

  std::stringstream ss;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
  }
  return ss.str();
}

bool AuthComponent::VerifyPassword(const std::string &password,
                                   const std::string &hash) {
  return HashPassword(password) == hash;
}

} // namespace components