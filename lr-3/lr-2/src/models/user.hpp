#pragma once

#include <cstdint>
#include <string>
#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>

namespace models::dto {

struct UserCreateRequest {
  std::string login;
  std::string password;
  std::string first_name;
  std::string last_name;
  std::string email;
};

struct UserResponse {
  int64_t id;
  std::string login;
  std::string first_name;
  std::string last_name;
  std::string email;
  std::string created_at;
};

userver::formats::json::Value
Serialize(const UserCreateRequest &data,
          userver::formats::serialize::To<userver::formats::json::Value>);

userver::formats::json::Value
Serialize(const UserResponse &data,
          userver::formats::serialize::To<userver::formats::json::Value>);

UserCreateRequest
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<UserCreateRequest>);

UserResponse
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<UserResponse>);
} // namespace models::dto