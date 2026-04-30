#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>

namespace models::dto {

struct ViewingCreateRequest {
  int64_t user_id;
  int64_t property_id;
  std::string scheduled_time;
  std::optional<std::string> comment;
};

struct ViewingResponse {
  int64_t id;
  int64_t property_id;
  int64_t user_id;
  std::string scheduled_time;
  std::string status;
};

userver::formats::json::Value
Serialize(const ViewingCreateRequest &data,
          userver::formats::serialize::To<userver::formats::json::Value>);

userver::formats::json::Value
Serialize(const ViewingResponse &data,
          userver::formats::serialize::To<userver::formats::json::Value>);

ViewingCreateRequest
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<ViewingCreateRequest>);

ViewingResponse
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<ViewingResponse>);

} // namespace models::dto