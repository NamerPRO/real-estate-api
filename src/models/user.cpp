#include "user.hpp"
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/utils/datetime/timepoint_tz.hpp>

namespace models::dto {

userver::formats::json::Value
Serialize(const UserCreateRequest &data,
          userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["login"] = data.login;
  builder["password"] = data.password;
  builder["first_name"] = data.first_name;
  builder["last_name"] = data.last_name;
  builder["email"] = data.email;
  return builder.ExtractValue();
}

userver::formats::json::Value
Serialize(const UserResponse &data,
          userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["id"] = data.id;
  builder["login"] = data.login;
  builder["first_name"] = data.first_name;
  builder["last_name"] = data.last_name;
  builder["email"] = data.email;
  builder["created_at"] = data.created_at;
  return builder.ExtractValue();
}

UserCreateRequest
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<UserCreateRequest>) {
  UserCreateRequest result;
  result.login = json["login"].As<std::string>();
  result.password = json["password"].As<std::string>();
  result.first_name = json["first_name"].As<std::string>();
  result.last_name = json["last_name"].As<std::string>();
  result.email = json["email"].As<std::string>();
  return result;
}

UserResponse
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<UserResponse>) {
  UserResponse result;
  result.id = json["id"].As<int64_t>();
  result.login = json["login"].As<std::string>();
  result.first_name = json["first_name"].As<std::string>();
  result.last_name = json["last_name"].As<std::string>();
  result.email = json["email"].As<std::string>();
  result.created_at = json["created_at"].As<userver::storages::postgres::TimePointTz>();

  return result;
}
} // namespace models::dto