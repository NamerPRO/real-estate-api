#pragma once

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/yaml_config/schema.hpp>

namespace auth::jwt {

class JwtChecker final
    : public userver::server::handlers::auth::AuthCheckerBase {
public:
  using AuthCheckResult = userver::server::handlers::auth::AuthCheckResult;

  JwtChecker(const std::string &secret);

  AuthCheckResult
  CheckAuth(const userver::server::http::HttpRequest &request,
            userver::server::request::RequestContext &context) const override;
  bool SupportsUserAuth() const noexcept override { return true; }

private:
  std::string secret_;
};

using JwtCheckerPtr = std::shared_ptr<JwtChecker>;

class JwtAuthComponent final
    : public userver::components::LoggableComponentBase {
public:
  static constexpr auto kName = "jwt-auth-checker";

  JwtAuthComponent(const userver::components::ComponentConfig &config,
                   const userver::components::ComponentContext &context);

  JwtCheckerPtr Get() const;

  static userver::yaml_config::Schema GetStaticConfigSchema();

private:
  JwtCheckerPtr authorizer_;
};

} // namespace auth::jwt