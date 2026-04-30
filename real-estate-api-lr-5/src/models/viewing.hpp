#pragma once

#include <chrono>
#include <cstdint>
#include <vector>
#include <string>
#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>
#include <userver/storages/postgres/io/chrono.hpp>

namespace models::dto {

struct Comment {
  std::string text;
  std::string author;
  std::chrono::system_clock::time_point timestamp;
};

struct ViewingCreateRequest {
  int64_t user_id;
  std::string property_id;
  std::chrono::system_clock::time_point scheduled_time;
  std::vector<Comment> comments;
};

struct ViewingResponse {
  std::string id;
  std::string property_id;
  int64_t user_id;
  std::chrono::system_clock::time_point scheduled_time;
  std::vector<Comment> comments;
  std::string status;
  std::chrono::system_clock::time_point created_at;
};

userver::formats::json::Value
Serialize(const Comment &data,
          userver::formats::serialize::To<userver::formats::json::Value>);

userver::formats::json::Value
Serialize(const ViewingCreateRequest &data,
          userver::formats::serialize::To<userver::formats::json::Value>);

userver::formats::json::Value
Serialize(const ViewingResponse &data,
          userver::formats::serialize::To<userver::formats::json::Value>);

Comment
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<Comment>);

ViewingCreateRequest
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<ViewingCreateRequest>);

ViewingResponse
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<ViewingResponse>);

} // namespace models::dto