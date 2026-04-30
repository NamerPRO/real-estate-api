#include "create_user_handler.hpp"
#include "../models/dto.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/server/http/http_status.hpp>

namespace handlers {

CreateUserHandler::CreateUserHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<components::StorageComponent>()),
      auth_(context.FindComponent<components::AuthComponent>()) {}

std::string CreateUserHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest &request,
    userver::server::request::RequestContext &) const {

  request.GetHttpResponse().SetContentType(
      userver::http::content_type::kApplicationJson);

  userver::formats::json::Value json;
  models::dto::UserCreateRequest dto;  
  try {
    const auto &body = request.RequestBody();
    json = userver::formats::json::FromString(body);
    dto = json.As<models::dto::UserCreateRequest>();
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

  int64_t user_id = storage_.CreateUser(dto, password_hash);

  if (user_id == components::StorageComponent::uniqueViolation) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
    models::dto::ErrorResponse error{"CONFLICT",
                                     "Login and/or email already exists"};
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{error}.ExtractValue());
  }

  if (user_id == components::StorageComponent::constraintViolation) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kUnprocessableEntity);
    models::dto::ErrorResponse error{"CONSTRAINTS_NOT_MATCHED",
                                     "Constraints are violated"};
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{error}.ExtractValue());
  }

  request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
  auto user = storage_.GetUserById(user_id);
  return userver::formats::json::ToString(
      userver::formats::json::ValueBuilder{*user}.ExtractValue());
}

} // namespace handlers