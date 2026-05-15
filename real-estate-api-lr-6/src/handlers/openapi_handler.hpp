#pragma once

#include <string>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/yaml_config/yaml_config.hpp>

namespace handlers {

class OpenApiHandler : public userver::server::handlers::HttpHandlerBase {
public:
  static constexpr std::string_view kName = "handler-openapi-spec";

  static userver::yaml_config::Schema GetStaticConfigSchema();

  explicit OpenApiHandler(const userver::components::ComponentConfig &config,
                          const userver::components::ComponentContext &context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest &request,
      userver::server::request::RequestContext &context) const override;

private:
  std::string openapi_spec_;
  std::string spec_file_path_;
};

} // namespace handlers