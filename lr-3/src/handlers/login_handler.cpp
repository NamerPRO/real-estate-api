#include "login_handler.hpp"
#include "../models/auth.hpp"
#include "../models/error.hpp"

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_status.hpp>

namespace handlers {

LoginHandler::LoginHandler(const userver::components::ComponentConfig &config,
                           const userver::components::ComponentContext &context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<components::StorageComponent>()),
      auth_(context.FindComponent<components::AuthComponent>()) {}

std::string LoginHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest &request,
    userver::server::request::RequestContext &) const {

  request.GetHttpResponse().SetContentType(
      userver::http::content_type::kApplicationJson);

  userver::formats::json::Value json;
  models::dto::LoginRequest dto;
  try {
    const auto &body = request.RequestBody();
    json = userver::formats::json::FromString(body);
    dto = json.As<models::dto::LoginRequest>();
  } catch (const std::exception &e) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
    models::dto::ErrorResponse error{"VALIDATION_ERROR",
                                     "Invalid request body"};
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{error}.ExtractValue());
  }

  if (dto.login.empty() || dto.password.empty()) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
    models::dto::ErrorResponse error{"VALIDATION_ERROR",
                                     "Login and password required"};
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{error}.ExtractValue());
  }

  std::string password_hash = auth_.HashPassword(dto.password);

  auto user_id_opt = storage_.VerifyCredentials(dto.login, password_hash);

  if (!user_id_opt) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
    models::dto::ErrorResponse error{"UNAUTHORIZED", "Invalid credentials"};
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{error}.ExtractValue());
  }

  auto user = storage_.GetUserById(*user_id_opt);
  if (!user) {
    request.SetResponseStatus(
        userver::server::http::HttpStatus::kInternalServerError);
    models::dto::ErrorResponse error{"INTERNAL_ERROR", "User data not found"};
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{error}.ExtractValue());
  }

  std::string token;
  if (auto existing_token = auth_.GetActiveToken(user->id)) {
    LOG_INFO() << "Returning existing active token for user_id=" << user->id;
    token = *existing_token;
  } else {
    token = auth_.GenerateToken(user->id, user->login);
  }

  request.SetResponseStatus(userver::server::http::HttpStatus::kOk);
  models::dto::AuthResponse response{token, "Bearer", user->id, user->login};

  return userver::formats::json::ToString(
      userver::formats::json::ValueBuilder{response}.ExtractValue());
}

} // namespace handlers