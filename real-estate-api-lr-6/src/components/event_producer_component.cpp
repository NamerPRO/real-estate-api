#include "event_producer_component.hpp"
#include <userver/components/component_context.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/datetime_light.hpp>
#include <userver/utils/uuid4.hpp>
#include <userver/urabbitmq/admin_channel.hpp>

namespace components {

  EventProducer::EventProducer(
      const userver::components::ComponentConfig &config,
      const userver::components::ComponentContext &context)
      : LoggableComponentBase(config, context),
        publisher_{
            context.FindComponent<userver::components::RabbitMQ>("rabbit")
                .GetClient()},
        exchange_{userver::urabbitmq::Exchange{"real_estate_events"}} {
    try
    {
      auto admin = publisher_->GetAdminChannel(
          userver::engine::Deadline::FromDuration(std::chrono::seconds(5)));

      auto queue = userver::urabbitmq::Queue(
          "real_estate_events");

      admin.DeclareQueue(
          queue,
          userver::engine::Deadline::FromDuration(std::chrono::seconds(5)));

      admin.DeclareExchange(
          exchange_,
          userver::engine::Deadline::FromDuration(std::chrono::seconds(5)));

      admin.BindQueue(
          exchange_,
          queue,
          "#",
          userver::engine::Deadline::FromDuration(std::chrono::seconds(5)));

      LOG_INFO() << "Queue real_estate_events declared and bound to real_estate_events exchange";
    } catch (const std::exception &e) {
      LOG_ERROR() << "Failed to declare and bind queue: " << e.what();
    }
  }

void EventProducer::PublishEvent(const std::string &event_type,
                                 const std::string &routing_key,
                                 const userver::formats::json::Value &payload) {
  auto event = userver::formats::json::MakeObject(
      "event_id", userver::utils::generators::GenerateUuid(), "event_type", event_type,
      "timestamp",
      userver::utils::datetime::Now(),
      "payload", payload);

  std::string body = userver::formats::json::ToString(event);

  try {
    publisher_->Publish(
        exchange_, routing_key, body,
        userver::engine::Deadline::FromDuration(std::chrono::seconds(5)));
    LOG_INFO() << "Event published: " << event_type << " [" << routing_key
               << "]";
  } catch (const std::exception &e) {
    LOG_ERROR() << "Failed to publish event " << event_type << ": " << e.what();
  }
}

void EventProducer::PublishUserRegistered(const models::dto::UserCreateRequest &user) {
  auto payload =
        userver::formats::json::ValueBuilder{user}.ExtractValue();
  PublishEvent("user.registered", "user.registered", payload);
}

void EventProducer::PublishPropertyCreated(const std::string &id, const models::dto::PropertyCreateRequest &property) {
  auto payload =
        userver::formats::json::ValueBuilder{property}.ExtractValue();
  PublishEvent("property.created", "property.created", userver::formats::json::MakeObject(
    "data", payload,
    "id", id
  ));
}

void EventProducer::PublishViewingScheduled(const models::dto::ViewingCreateRequest &viewing) {
  auto payload =
        userver::formats::json::ValueBuilder{viewing}.ExtractValue();
  PublishEvent("viewing.scheduled", "viewing.scheduled", payload);
}

} // namespace components