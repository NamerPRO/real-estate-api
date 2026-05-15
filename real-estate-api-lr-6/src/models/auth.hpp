#pragma once

#include <cstdint>
#include <string>
#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>

namespace models::dto {

struct LoginRequest {
  std::string login;
  std::string password;
};

struct AuthResponse {
  std::string access_token;
  std::string token_type;
  int64_t user_id;
  std::string login;
};

struct AuthTokenData {
  int64_t user_id;
  std::string login;
  bool is_authenticated;
};

userver::formats::json::Value
Serialize(const LoginRequest &data,
          userver::formats::serialize::To<userver::formats::json::Value>);

userver::formats::json::Value
Serialize(const AuthResponse &data,
          userver::formats::serialize::To<userver::formats::json::Value>);

LoginRequest
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<LoginRequest>);

AuthResponse
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<AuthResponse>);

} // namespace models::dto