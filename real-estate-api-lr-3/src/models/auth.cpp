#include "auth.hpp"

namespace models::dto {

userver::formats::json::Value
Serialize(const RegisterRequest &data,
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
Serialize(const LoginRequest &data,
          userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["login"] = data.login;
  builder["password"] = data.password;
  return builder.ExtractValue();
}

userver::formats::json::Value
Serialize(const AuthResponse &data,
          userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["access_token"] = data.access_token;
  builder["token_type"] = data.token_type;
  builder["user_id"] = data.user_id;
  builder["login"] = data.login;
  return builder.ExtractValue();
}

RegisterRequest
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<RegisterRequest>) {
  RegisterRequest result;
  result.login = json["login"].As<std::string>();
  result.password = json["password"].As<std::string>();
  result.first_name = json["first_name"].As<std::string>();
  result.last_name = json["last_name"].As<std::string>();
  result.email = json["email"].As<std::string>();
  return result;
}

LoginRequest Parse(const userver::formats::json::Value &json,
                   userver::formats::parse::To<LoginRequest>) {
  LoginRequest result;
  result.login = json["login"].As<std::string>();
  result.password = json["password"].As<std::string>();
  return result;
}

AuthResponse Parse(const userver::formats::json::Value &json,
                   userver::formats::parse::To<AuthResponse>) {
  AuthResponse result;
  result.access_token = json["access_token"].As<std::string>();
  result.token_type = json["token_type"].As<std::string>();
  result.user_id = json["user_id"].As<int64_t>();
  result.login = json["login"].As<std::string>();
  return result;
}

} // namespace models::dto