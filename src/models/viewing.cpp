#include "viewing.hpp"

#include <chrono>
#include <string>
#include <userver/formats/bson/value_builder.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/storages/postgres/io/chrono.hpp>

namespace models::dto {

// ===================== SERIALIZE (comment -> json) =====================

userver::formats::json::Value
Serialize(const Comment &data,
          userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;

  builder["text"] = data.text;
  builder["author"] = data.author;
  builder["timestamp"] = data.timestamp;

  return builder.ExtractValue();
}

// ===================== SERIALIZE (request -> json) =====================

userver::formats::json::Value
Serialize(const ViewingCreateRequest &data,
          userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;

  builder["user_id"] = data.user_id;
  builder["property_id"] = data.property_id;
  builder["scheduled_time"] = data.scheduled_time;
  
  for (const auto &comment : data.comments) {
    builder["comments"].PushBack(comment);
  }

  return builder.ExtractValue();
}

// ===================== SERIALIZE (response -> json) =====================

userver::formats::json::Value
Serialize(const ViewingResponse &data,
          userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;

  builder["id"] = data.id;
  builder["property_id"] = data.property_id;
  builder["user_id"] = data.user_id;
  builder["scheduled_time"] = data.scheduled_time;
  builder["status"] = data.status;

  for (const auto &comment : data.comments) {
    builder["comments"].PushBack(comment);
  }

  builder["created_at"] = data.created_at;

  return builder.ExtractValue();
}

// ===================== PARSE (json -> comment) =====================

Comment Parse(const userver::formats::json::Value &json,
              userver::formats::parse::To<Comment>) {
  Comment result;

  result.text = json["text"].As<std::string>();
  result.author = json["author"].As<std::string>();
  result.timestamp =
      json["timestamp"].As<std::chrono::system_clock::time_point>();

  return result;
}

// ===================== PARSE (json -> request) =====================

ViewingCreateRequest Parse(const userver::formats::json::Value &json,
                           userver::formats::parse::To<ViewingCreateRequest>) {
  ViewingCreateRequest result;

  result.user_id = json["user_id"].As<int64_t>();

  result.property_id = json["property_id"].As<std::string>();

  result.scheduled_time =
      json["scheduled_time"].As<std::chrono::system_clock::time_point>();

  for (const auto &comment_json : json["comments"]) {
    result.comments.push_back(comment_json.As<Comment>());
  }

  return result;
}

// ===================== PARSE (json -> response) =====================

ViewingResponse Parse(const userver::formats::json::Value &json,
                      userver::formats::parse::To<ViewingResponse>) {
  ViewingResponse result;

  result.id = json["id"].As<int64_t>();
  result.property_id = json["property_id"].As<std::string>();
  result.user_id = json["user_id"].As<int64_t>();

  result.scheduled_time =
      json["scheduled_time"].As<std::chrono::system_clock::time_point>();

  result.status = json["status"].As<std::string>();

  for (const auto &comment_json : json["comments"]) {
    result.comments.push_back(comment_json.As<Comment>());
  }

  result.created_at =
      json["created_at"].As<std::chrono::system_clock::time_point>();

  return result;
}

} // namespace models::dto