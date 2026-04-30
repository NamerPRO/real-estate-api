#include "property.hpp"
#include <chrono>
#include <date/date.h>
#include <string>
#include <userver/formats/bson/binary.hpp>
#include <userver/formats/json/parser/parser.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/utils/datetime/from_string_saturating.hpp>
#include <userver/utils/datetime/timepoint_tz.hpp>
#include <userver/utils/datetime_light.hpp>

namespace models::dto {

userver::formats::json::Value
Serialize(const PropertyCreateRequest &data,
          userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["owner_id"] = data.owner_id;
  builder["type"] = data.type;
  builder["title"] = data.title;
  builder["city"] = data.city;
  builder["address"] = data.address;
  builder["details"] = data.details;
  builder["price"] = data.price;
  builder["features"] = data.features;
  builder["status"] = data.status;
  return builder.ExtractValue();
}

userver::formats::json::Value
Serialize(const PropertyResponse &data,
          userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["id"] = data.id;
  builder["owner_id"] = data.owner_id;
  builder["type"] = data.type;
  builder["title"] = data.title;
  builder["city"] = data.city;
  builder["address"] = data.address;
  builder["details"] = data.details;
  builder["price"] = data.price;
  builder["features"] = data.features;
  builder["status"] = data.status;
  builder["created_at"] = data.created_at;
  return builder.ExtractValue();
}

userver::formats::json::Value
Serialize(const PropertyUpdateRequest &data,
          userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  if (data.status) {
    builder["status"] = *data.status;
  }
  if (data.price) {
    builder["price"] = *data.price;
  }
  return builder.ExtractValue();
}

PropertyCreateRequest
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<PropertyCreateRequest>) {
  PropertyCreateRequest result;
  result.owner_id = json["owner_id"].As<int64_t>();
  result.type = json["type"].As<std::string>();
  result.title = json["title"].As<std::string>();
  result.city = json["city"].As<std::string>();

  result.address = json["address"];
  result.details = json["details"];

  result.price = json["price"].As<double>();
  
  if (json.HasMember("features")) {
    result.features = json["features"];
  }

  result.status = json["status"].As<std::string>();
  
  return result;
}

PropertyResponse
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<PropertyResponse>) {
  PropertyResponse result;
  result.id = json["id"].As<std::string>();
  result.owner_id = json["owner_id"].As<int64_t>();
  result.title = json["title"].As<std::string>();
  result.city = json["city"].As<std::string>();

  result.address =  json["address"];
  result.details = json["details"];

  result.price = json["price"].As<double>();

  if (json.HasMember("features")) {
    result.features = json["features"];
  }

  result.status = json["status"].As<std::string>();
  result.created_at = json["created_at"].As<std::chrono::system_clock::time_point>();

  return result;
}

PropertyUpdateRequest
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<PropertyUpdateRequest>) {
  PropertyUpdateRequest result;
  if (json.HasMember("status")) {
    result.status = json["status"].As<std::string>();
  }
  if (json.HasMember("price")) {
    result.price = json["price"].As<double>();
  }
  return result;
}

} // namespace models::dto