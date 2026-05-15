#pragma once

#include <string>
#include <userver/components/component_base.hpp>
#include <userver/formats/json.hpp>
#include <userver/urabbitmq/client.hpp>
#include <userver/urabbitmq/component.hpp>
#include <userver/urabbitmq/typedefs.hpp>

#include "../models/dto.hpp"

namespace components {

class EventProducer final : public userver::components::LoggableComponentBase {
public:
  static constexpr std::string_view kName = "event-producer";

  EventProducer(const userver::components::ComponentConfig &config,
                const userver::components::ComponentContext &context);

  void PublishUserRegistered(const models::dto::UserCreateRequest &user);
  void PublishPropertyCreated(const std::string& id, const models::dto::PropertyCreateRequest &property);
  void PublishViewingScheduled(const models::dto::ViewingCreateRequest &viewing);

private:
  void PublishEvent(const std::string &event_type,
                    const std::string &routing_key,
                    const userver::formats::json::Value &payload);

  std::shared_ptr<userver::urabbitmq::Client> publisher_;
  userver::urabbitmq::Exchange exchange_;
};

} // namespace components