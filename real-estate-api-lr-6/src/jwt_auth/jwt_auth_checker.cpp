#include "jwt_auth_checker.hpp"

#include <jwt-cpp/jwt.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace auth::jwt {

namespace {
static constexpr std::string_view kSecret = "secret";
static constexpr std::string_view kAlgorithm = "Bearer ";
static constexpr const char *kServiceName = "sample";
static constexpr std::string_view kIssuer = "real-estate-api";

JwtChecker::AuthCheckResult
MakeErrorResult(JwtChecker::AuthCheckResult::Status status,
                std::string message) {
  userver::server::handlers::HandlerErrorCode error_code =
      userver::server::handlers::HandlerErrorCode::kInvalidUsage;
  switch (status) {
  case JwtChecker::AuthCheckResult::Status::kTokenNotFound:
  case JwtChecker::AuthCheckResult::Status::kInvalidToken:
    error_code = userver::server::handlers::HandlerErrorCode::kUnauthorized;
    break;
  case JwtChecker::AuthCheckResult::Status::kForbidden:
    error_code = userver::server::handlers::HandlerErrorCode::kForbidden;
    break;
  default:
    break;
  }
  return JwtChecker::AuthCheckResult{status, {}, std::move(message), error_code};
}
} // namespace

JwtChecker::JwtChecker(const std::string &secret) : secret_(secret) {}

JwtChecker::AuthCheckResult
JwtChecker::CheckAuth(const userver::server::http::HttpRequest &request,
                      userver::server::request::RequestContext &context) const {
  const std::string_view auth_header =
      request.GetHeader(userver::http::headers::kAuthorization);

  if (auth_header.empty()) {
    request.GetHttpResponse().SetContentType(
        userver::http::content_type::kTextPlain);
    return MakeErrorResult(AuthCheckResult::Status::kTokenNotFound,
                           "Missing 'Authorization' header");
  }

  if (!auth_header.starts_with(kAlgorithm)) {
    request.GetHttpResponse().SetContentType(
        userver::http::content_type::kTextPlain);
    return MakeErrorResult(AuthCheckResult::Status::kInvalidToken,
                           "Invalid authorization type, expected 'Bearer'");
  }

  const std::string_view token = auth_header.substr(kAlgorithm.length());

  try {
    auto decoded = ::jwt::decode(std::string(token));

    auto verifier = ::jwt::verify()
                        .allow_algorithm(::jwt::algorithm::hs256{secret_})
                        .with_issuer(std::string{kIssuer});

    verifier.verify(decoded);

    if (!decoded.has_payload_claim("user_id") ||
        !decoded.has_payload_claim("login")) {
      request.GetHttpResponse().SetContentType(
          userver::http::content_type::kTextPlain);
      return MakeErrorResult(AuthCheckResult::Status::kInvalidToken,
                             "Token missing required claims: user_id or login");
    }

    auto user_id_claim = decoded.get_payload_claim("user_id");
    auto login_claim = decoded.get_payload_claim("login");

    context.SetData("user_id", user_id_claim.as_string());
    context.SetData("login", login_claim.as_string());

    return AuthCheckResult{
        AuthCheckResult::Status::kOk,
        std::optional<std::string>{user_id_claim.as_string()},
        {}};

  } catch (const ::jwt::error::token_verification_exception &exc) {
    request.GetHttpResponse().SetContentType(
        userver::http::content_type::kTextPlain);
    return MakeErrorResult(AuthCheckResult::Status::kInvalidToken,
                           "Token verification failed: " +
                               std::string{exc.what()});
  } catch (const std::exception &exc) {
    request.GetHttpResponse().SetContentType(
        userver::http::content_type::kTextPlain);
    return MakeErrorResult(AuthCheckResult::Status::kForbidden,
                           "Token processing error: " +
                               std::string{exc.what()});
  }
}

JwtAuthComponent::JwtAuthComponent(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : LoggableComponentBase(config, context) {
  authorizer_ = std::make_shared<JwtChecker>(config[kSecret].As<std::string>());
}

JwtCheckerPtr JwtAuthComponent::Get() const { return authorizer_; }

userver::yaml_config::Schema JwtAuthComponent::GetStaticConfigSchema() {
  return userver::yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: JWT Auth Checker Component
additionalProperties: false
properties:
    secret:
        type: string
        description: secret key for JWT validation
)");
}

} // namespace auth::jwt