#include "event_consumer_component.hpp"
#include "read_database_component.hpp"
#include <chrono>
#include <cstdint>
#include <string>
#include <userver/components/component_context.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/formats/json.hpp>
#include <userver/logging/log.hpp>
#include <userver/urabbitmq/typedefs.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/engine/task/current_task.hpp>

namespace components {

EventConsumer::EventConsumer(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : ConsumerComponentBase(config, context),
      read_db_{context.FindComponent<ReadDatabaseComponent>(
          "read-database-component")} {}

void EventConsumer::Process(std::string message) {
  try {
    auto json = userver::formats::json::FromString(message);

    std::string event_type = "unknown";
    if (json.HasMember("event_type")) {
      event_type = json["event_type"].As<std::string>();
    }

    LOG_INFO() << "Consumed event: " << event_type;

    auto processor = handlers.find(event_type);
    if (processor == handlers.end()) {
      LOG_ERROR() << "Cannot find processor for " << event_type;
      return;
    }
    processor->second(json["payload"]);
    IndexingServiceStub(event_type, json["payload"]);
  } catch (const std::exception &e) {
    LOG_ERROR() << "Error processing message: " << e.what();
  }
}

void EventConsumer::PropertyCreated(const userver::formats::json::Value &propertyJson) {
  const auto &property = propertyJson["data"].As<models::dto::PropertyCreateRequest>();
  LOG_INFO() << "Created " << property.type << " in " << property.city << ". Title: " << property.title;
}

void EventConsumer::UserRegistered(const userver::formats::json::Value &userJson) {
  const auto &user = userJson.As<models::dto::UserCreateRequest>();
  LOG_INFO() << "User " << user.first_name << " " << user.last_name << " registered under " << user.login << " login";
}

void EventConsumer::ViewingScheduled(
    const userver::formats::json::Value &viewingJson) {
  const auto &viewing = viewingJson.As<models::dto::ViewingCreateRequest>();
  LOG_INFO() << "User with id " << viewing.user_id << " scheduled property with id " << viewing.property_id << " at time " << viewing.scheduled_time;
}

void EventConsumer::IndexingServiceStub(
    const std::string &event_type,
    const userver::formats::json::Value &json) {
  if (event_type == "property.created") {
    const auto &property = json["data"].As<models::dto::PropertyCreateRequest>();
    const std::string &id = json["id"].As<std::string>();
    read_db_.CreateProperty(id, property);
  } else if (event_type == "user.registered") {
    const auto &user = json.As<models::dto::UserCreateRequest>();
    read_db_.CreateUser(user, user.password);
  } else if (event_type == "viewing.scheduled") {
    const auto &viewing = json.As<models::dto::ViewingCreateRequest>();
    read_db_.CreateViewing(viewing);
  } else {
    LOG_ERROR() << "Error migrating data to read database. Event type unrecognised...";
  }
}

} // namespace components