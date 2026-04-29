#pragma once

#include "../components/mongo_storage_component.hpp"
#include "../components/redis_cache_component.hpp"

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

namespace handlers {

class GetPropertiesHandler : public userver::server::handlers::HttpHandlerBase {
public:
  static constexpr std::string_view kName = "handler-get-properties";

  GetPropertiesHandler(const userver::components::ComponentConfig &config,
                       const userver::components::ComponentContext &context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest &request,
      userver::server::request::RequestContext &context) const override;

private:
  components::MongoStorageComponent &storage_;
  components::RedisCacheComponent &cache_;
};

} // namespace handlers