#pragma once

#include <userver/components/component_base.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/urabbitmq/client.hpp>
#include <userver/urabbitmq/component.hpp>
#include <userver/urabbitmq/consumer_component_base.hpp>

#include "../models/dto.hpp"
#include "read_database_component.hpp"

namespace components {

using namespace models::dto;

class EventConsumer final : public userver::urabbitmq::ConsumerComponentBase {
public:
  static constexpr std::string_view kName = "event-consumer";

  EventConsumer(const userver::components::ComponentConfig &config,
                const userver::components::ComponentContext &context);

  std::vector<std::string> GetConsumedMessages();
  size_t GetConsumedMessagesCount();
  void ClearConsumedMessages();

protected:
  void Process(std::string message) override;

private:
  void PropertyCreated(const userver::formats::json::Value &propertyJson);
  void UserRegistered(const userver::formats::json::Value &userJson);
  void ViewingScheduled(const userver::formats::json::Value &viewingJson);
  void IndexingServiceStub(const std::string &event_type, const userver::formats::json::Value &json);

  std::unordered_map<std::string,
                     std::function<void(const userver::formats::json::Value &)>>
  handlers = {{"property.created",
                [this](const userver::formats::json::Value &event) {
                  PropertyCreated(event);
                }},
              {"user.registered",
                [this](const userver::formats::json::Value &event) {
                  UserRegistered(event);
                }},
              {"viewing.scheduled",
                [this](const userver::formats::json::Value &event) {
                  ViewingScheduled(event);
                }}
  };

  ReadDatabaseComponent &read_db_;

};

} // namespace components