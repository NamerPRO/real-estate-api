#include "viewing.hpp"

namespace models::dto {

userver::formats::json::Value
Serialize(const ViewingCreateRequest &data,
          userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["user_id"] = data.user_id;
  builder["property_id"] = data.property_id;
  builder["scheduled_time"] = data.scheduled_time;
  if (data.comment) {
    builder["comment"] = *data.comment;
  }
  return builder.ExtractValue();
}

userver::formats::json::Value
Serialize(const ViewingResponse &data,
          userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["id"] = data.id;
  builder["property_id"] = data.property_id;
  builder["user_id"] = data.user_id;
  builder["scheduled_time"] = data.scheduled_time;
  builder["status"] = data.status;
  return builder.ExtractValue();
}

ViewingCreateRequest
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<ViewingCreateRequest>) {
  ViewingCreateRequest result;
  result.user_id = json["user_id"].As<int64_t>();
  result.property_id = json["property_id"].As<int64_t>();
  result.scheduled_time = json["scheduled_time"].As<std::string>();
  if (json.HasMember("comment")) {
    result.comment = json["comment"].As<std::string>();
  }
  return result;
}

ViewingResponse
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<ViewingResponse>) {
  ViewingResponse result;
  result.id = json["id"].As<int64_t>();
  result.property_id = json["property_id"].As<int64_t>();
  result.user_id = json["user_id"].As<int64_t>();
  result.scheduled_time = json["scheduled_time"].As<std::string>();
  result.status = json["status"].As<std::string>();
  return result;
}

} // namespace models::dto