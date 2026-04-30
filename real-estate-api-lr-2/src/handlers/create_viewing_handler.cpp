#include "create_viewing_handler.hpp"
#include "../models/dto.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/server/http/http_status.hpp>

namespace handlers {

CreateViewingHandler::CreateViewingHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<components::StorageComponent>()) {}

std::string CreateViewingHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest &request,
    userver::server::request::RequestContext &) const {

  const auto &body = request.RequestBody();
  auto json = userver::formats::json::FromString(body);

  request.GetHttpResponse().SetContentType(
      userver::http::content_type::kApplicationJson);

  models::dto::ViewingCreateRequest dto;
  try {
    dto = json.As<models::dto::ViewingCreateRequest>();
  } catch (const std::exception &e) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
    models::dto::ErrorResponse error{"VALIDATION_ERROR",
                                     "Invalid request body"};
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{error}.ExtractValue());
  }

  int64_t viewing_id = storage_.CreateViewing(dto);

  if (viewing_id == -1) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
    models::dto::ErrorResponse error{"NOT_FOUND", "Property not found"};
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{error}.ExtractValue());
  }

  request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
  models::dto::ViewingResponse response{viewing_id, dto.property_id,
                                        dto.user_id, dto.scheduled_time,
                                        "scheduled"};
  return userver::formats::json::ToString(
      userver::formats::json::ValueBuilder{response}.ExtractValue());
}

} // namespace handlers