#include "property.hpp"

namespace models::dto {

userver::formats::json::Value
Serialize(const PropertyCreateRequest &data,
          userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["owner_id"] = data.owner_id;
  builder["title"] = data.title;
  builder["description"] = data.description;
  builder["city"] = data.city;
  builder["address"] = data.address;
  builder["price"] = data.price;
  builder["status"] = data.status;
  return builder.ExtractValue();
}

userver::formats::json::Value
Serialize(const PropertyResponse &data,
          userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["id"] = data.id;
  builder["owner_id"] = data.owner_id;
  builder["title"] = data.title;
  builder["city"] = data.city;
  builder["price"] = data.price;
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
  result.title = json["title"].As<std::string>();
  result.description = json["description"].As<std::string>();
  result.city = json["city"].As<std::string>();
  result.address = json["address"].As<std::string>();
  result.price = json["price"].As<double>();
  result.status = json["status"].As<std::string>();
  return result;
}

PropertyResponse
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<PropertyResponse>) {
  PropertyResponse result;
  result.id = json["id"].As<int64_t>();
  result.owner_id = json["owner_id"].As<int64_t>();
  result.title = json["title"].As<std::string>();
  result.city = json["city"].As<std::string>();
  result.price = json["price"].As<double>();
  result.status = json["status"].As<std::string>();
  result.created_at = json["created_at"].As<std::string>();
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