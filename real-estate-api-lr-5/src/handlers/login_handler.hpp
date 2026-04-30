#pragma once

#include "../components/auth_component.hpp"
#include "../components/postgres_storage_component.hpp"
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

namespace handlers {

class LoginHandler : public userver::server::handlers::HttpHandlerBase {
public:
  static constexpr std::string_view kName = "handler-login";

  LoginHandler(const userver::components::ComponentConfig &config,
               const userver::components::ComponentContext &context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest &request,
      userver::server::request::RequestContext &context) const override;

private:
  components::PostgresStorageComponent &storage_;
  components::AuthComponent &auth_;
};

} // namespace handlers