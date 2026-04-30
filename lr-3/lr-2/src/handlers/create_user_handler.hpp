#pragma once

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include "../components/storage_component.hpp"

namespace handlers {

class CreateUserHandler : public userver::server::handlers::HttpHandlerBase {
public:
  static constexpr std::string_view kName = "handler-create-user";

  CreateUserHandler(const userver::components::ComponentConfig &config,
                    const userver::components::ComponentContext &context);

  std::string
  HandleRequestThrow(const userver::server::http::HttpRequest &request,
                     userver::server::request::RequestContext &) const override;

private:
  components::StorageComponent &storage_;
};

} // namespace handlers