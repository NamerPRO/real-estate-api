#pragma once

#include <cstdint>
#include <string>
#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>

namespace models::dto {

struct PropertyCreateRequest {
  int64_t owner_id;
  std::string title;
  std::string description;
  std::string city;
  std::string address;
  double price;
  std::string status;
};

struct PropertyResponse {
  int64_t id;
  int64_t owner_id;
  std::string title;
  std::string city;
  double price;
  std::string status;
  std::string created_at;
};

struct PropertyUpdateRequest {
  std::optional<std::string> status;
  std::optional<double> price;
};

userver::formats::json::Value
Serialize(const PropertyCreateRequest &data,
          userver::formats::serialize::To<userver::formats::json::Value>);

userver::formats::json::Value
Serialize(const PropertyResponse &data,
          userver::formats::serialize::To<userver::formats::json::Value>);

userver::formats::json::Value
Serialize(const PropertyUpdateRequest &data,
          userver::formats::serialize::To<userver::formats::json::Value>);

PropertyCreateRequest
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<PropertyCreateRequest>);

PropertyResponse
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<PropertyResponse>);

PropertyUpdateRequest
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<PropertyUpdateRequest>);
} // namespace models::dto