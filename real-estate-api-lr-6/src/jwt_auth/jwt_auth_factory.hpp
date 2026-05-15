#pragma once

#include <userver/server/handlers/auth/auth_checker_factory.hpp>

#include "jwt_auth_checker.hpp"

namespace auth::jwt {

class JwtAuthCheckerFactory final
    : public userver::server::handlers::auth::AuthCheckerFactoryBase {
public:
  static constexpr const char *kAuthType = "jwt-auth";

  explicit JwtAuthCheckerFactory(
      const userver::components::ComponentContext &context);

  userver::server::handlers::auth::AuthCheckerBasePtr
  MakeAuthChecker(const userver::server::handlers::auth::HandlerAuthConfig &)
      const override;

private:
  auth::jwt::JwtAuthComponent &auth_;
};

} // namespace auth::jwt