#pragma once

#include <cstdint>
#include <string>
#include <userver/formats/bson/document.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/datetime/timepoint_tz.hpp>

namespace models::dto {

struct PropertyCreateRequest {
  int64_t owner_id;
  std::string type;
  std::string title;
  std::string city;

  userver::formats::json::Value address;
  userver::formats::json::Value details;

  double price;

  userver::formats::json::Value features;

  std::string status;
};

struct PropertyResponse {
  std::string id;
  int64_t owner_id;
  std::string type;
  std::string title;
  std::string city;

  userver::formats::json::Value address;
  userver::formats::json::Value details;

  double price;

  userver::formats::json::Value features;

  std::string status;
  std::chrono::system_clock::time_point created_at;
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