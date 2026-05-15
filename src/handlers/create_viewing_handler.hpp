#pragma once

#include "../components/mongo_storage_component.hpp"
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include "../components/event_producer_component.hpp"

namespace handlers {

class CreateViewingHandler : public userver::server::handlers::HttpHandlerBase {
public:
  static constexpr std::string_view kName = "handler-create-viewing";

  CreateViewingHandler(const userver::components::ComponentConfig &config,
                       const userver::components::ComponentContext &context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest &request,
      userver::server::request::RequestContext &context) const override;

private:
  components::MongoStorageComponent &storage_;
  components::EventProducer &producer_;
};

} // namespace handlers