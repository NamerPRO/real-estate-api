#include "register_handler.hpp"
#include "../models/auth.hpp"
#include "../models/error.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_status.hpp>

namespace handlers {

RegisterHandler::RegisterHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<components::StorageComponent>()),
      auth_(context.FindComponent<components::AuthComponent>()) {}

std::string RegisterHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest &request,
    userver::server::request::RequestContext &) const {

  const auto &body = request.RequestBody();
  auto json = userver::formats::json::FromString(body);

  request.GetHttpResponse().SetContentType(
      userver::http::content_type::kApplicationJson);

  models::dto::RegisterRequest dto;
  try {
    dto = json.As<models::dto::RegisterRequest>();
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

  if (dto.email.empty()) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
    models::dto::ErrorResponse error{"VALIDATION_ERROR", "Email required"};
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{error}.ExtractValue());
  }

  std::string password_hash = auth_.HashPassword(dto.password);

  models::dto::UserCreateRequest user_request{
      dto.login,
      "",
      dto.first_name, dto.last_name, dto.email};

  int64_t user_id = storage_.RegisterUser(user_request, password_hash);

  if (user_id == -1) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
    models::dto::ErrorResponse error{"CONFLICT", "Login already exists"};
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{error}.ExtractValue());
  }

  request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
  auto user = storage_.GetUserById(user_id);

  return userver::formats::json::ToString(
      userver::formats::json::ValueBuilder{*user}.ExtractValue());
}

} // namespace handlers