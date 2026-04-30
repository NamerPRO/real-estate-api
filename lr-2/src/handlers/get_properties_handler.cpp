#include "get_properties_handler.hpp"
#include "../models/dto.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/server/http/http_status.hpp>

namespace handlers {

GetPropertiesHandler::GetPropertiesHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<components::StorageComponent>()) {}

std::string GetPropertiesHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest &request,
    userver::server::request::RequestContext &) const {

  request.GetHttpResponse().SetContentType(
      userver::http::content_type::kApplicationJson);

  if (request.HasArg("city")) {
    auto properties = storage_.GetPropertiesByCity(request.GetArg("city"));
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{properties}.ExtractValue());
  }

  if (request.HasArg("min_price") && request.HasArg("max_price")) {
    try {
      double min = std::stod(request.GetArg("min_price"));
      double max = std::stod(request.GetArg("max_price"));
      auto properties = storage_.GetPropertiesByPriceRange(min, max);
      return userver::formats::json::ToString(
          userver::formats::json::ValueBuilder{properties}.ExtractValue());
    } catch (const std::exception &e) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      models::dto::ErrorResponse error{"VALIDATION_ERROR",
                                       "Invalid price format"};
      return userver::formats::json::ToString(
          userver::formats::json::ValueBuilder{error}.ExtractValue());
    }
  }

  request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
  models::dto::ErrorResponse error{
      "BAD_REQUEST", "Missing query parameters (city or min_price/max_price)"};
  return userver::formats::json::ToString(
      userver::formats::json::ValueBuilder{error}.ExtractValue());
}

} // namespace handlers