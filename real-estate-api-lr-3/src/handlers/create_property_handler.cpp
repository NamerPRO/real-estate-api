#include "create_property_handler.hpp"
#include "../models/dto.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/http/http_status.hpp>

namespace handlers {

CreatePropertyHandler::CreatePropertyHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<components::StorageComponent>()) {}

std::string CreatePropertyHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest &request,
    userver::server::request::RequestContext &) const {

  request.GetHttpResponse().SetContentType(
      userver::http::content_type::kApplicationJson);

  userver::formats::json::Value json;
  models::dto::PropertyCreateRequest dto;
  try {
    const auto &body = request.RequestBody();
    json = userver::formats::json::FromString(body);
    dto = json.As<models::dto::PropertyCreateRequest>();
  } catch (const std::exception &e) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
    models::dto::ErrorResponse error{"VALIDATION_ERROR",
                                     "Invalid request body"};
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{error}.ExtractValue());
  }

  int64_t property_id = storage_.CreateProperty(dto);

  if (property_id == components::StorageComponent::dataViolation) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
    models::dto::ErrorResponse error{"NOT_FOUND", "Owner not found"};
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{error}.ExtractValue());
  }

  if (property_id == components::StorageComponent::constraintViolation) {
    request.SetResponseStatus(
        userver::server::http::HttpStatus::kUnprocessableEntity);
    models::dto::ErrorResponse error{"CONSTRAINTS_NOT_MATCHED",
                                     "Constraints are violated"};
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{error}.ExtractValue());
  }

  request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
  auto property = storage_.GetPropertyById(property_id);
  return userver::formats::json::ToString(
      userver::formats::json::ValueBuilder{*property}.ExtractValue());
}

} // namespace handlers