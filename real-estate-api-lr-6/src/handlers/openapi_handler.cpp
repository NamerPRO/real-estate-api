#include "openapi_handler.hpp"
#include <fstream>
#include <sstream>
#include <userver/components/component_config.hpp>
#include <userver/http/content_type.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace handlers {

userver::yaml_config::Schema
OpenApiHandler::GetStaticConfigSchema() {
  return userver::yaml_config::MergeSchemas<userver::server::handlers::HttpHandlerBase>(R"(
type: object
description: OpenAPI Specification Handler Configuration
additionalProperties: false
properties:
    spec_file_path:
        type: string
        description: Path to the OpenAPI YAML specification file
        default: "./configs/openapi.yaml"
)");
}

OpenApiHandler::OpenApiHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerBase(config, context) {
  spec_file_path_ = config["spec_file_path"].As<std::string>("./configs/openapi.yaml");

  std::ifstream file(spec_file_path_);
  if (file.is_open()) {
    std::stringstream buffer;
    buffer << file.rdbuf();
    openapi_spec_ = buffer.str();
    LOG_INFO() << "Loaded OpenAPI spec from: " << spec_file_path_;
  } else {
    LOG_WARNING() << "Failed to load OpenAPI spec from: " << spec_file_path_;
  }
}

std::string OpenApiHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest &request,
    userver::server::request::RequestContext &) const {

  if (openapi_spec_.empty()) {
    request.GetHttpResponse().SetContentType(
        userver::http::content_type::kTextPlain);
    request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
    return "OpenAPI spec not found";
  }

  request.SetResponseStatus(userver::server::http::HttpStatus::kOk);
  request.GetHttpResponse().SetContentType("application/x-yaml");

  return openapi_spec_;
}

} // namespace handlers