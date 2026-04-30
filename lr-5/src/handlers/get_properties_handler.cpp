#include "get_properties_handler.hpp"
#include "../models/dto.hpp"

#include <fmt/format.h>
#include <string>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/server/http/http_status.hpp>

#include <vector>

namespace handlers {

GetPropertiesHandler::GetPropertiesHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<components::MongoStorageComponent>()),
      cache_(context.FindComponent<components::RedisCacheComponent>()),
      limiter_(context.FindComponent<components::RateLimiterComponent>()) {}

void GetPropertiesHandler::SetRateLimiterHeaders(
  userver::server::http::HttpResponse &response,
  const std::string &ip) const {
  response.SetHeader(std::string_view("X-RateLimit-Limit"),
                     std::to_string(components::RateLimiterComponent::limit));
  response.SetHeader(std::string_view("X-RateLimit-Remaining"), std::to_string(limiter_.GetRemaining(ip)));

  const auto now = userver::utils::datetime::Now();
  const auto reset_time =
      now + std::chrono::seconds(components::RateLimiterComponent::timeSec);

  response.SetHeader(
      std::string_view("X-RateLimit-Reset"),
      std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                         reset_time.time_since_epoch())
                         .count()));

  response.SetHeader(std::string_view("Retry-After"),
                     std::to_string(components::RateLimiterComponent::timeSec));
}

std::string GetPropertiesHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest &request,
    userver::server::request::RequestContext &) const {

  request.GetHttpResponse().SetContentType(
      userver::http::content_type::kApplicationJson);

  const auto &ip = request.GetRemoteAddress().PrimaryAddressString();
  
  bool isLimitExpired = !limiter_.CheckRateLimit(ip);
  SetRateLimiterHeaders(request.GetHttpResponse(), ip);

  if (isLimitExpired) {
    request.SetResponseStatus(userver::server::http::HttpStatus::TooManyRequests);
    models::dto::ErrorResponse error{"Too Many Requests",
                                     "Too many requst were made"};
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{error}.ExtractValue());
  }

  int from, to;
  try {
    from = std::stoi(request.GetArg("from"));
    to = std::stoi(request.GetArg("to"));
    if (from <= 0 || to <= 0 || to < from) {
      throw std::invalid_argument("Invalid from/to bounds!");
    }
  } catch (const std::exception &e) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
    models::dto::ErrorResponse error{"BAD_REQUEST", e.what()};
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{error}.ExtractValue());
  }

  if (request.HasArg("city")) {
    const auto city = request.GetArg("city");
    auto properties = cache_.SearchPropertiesByCity(city, from, to);
    if (!properties.has_value()) {
      properties = storage_.GetPropertiesByCity(city, to, from);
      cache_.SaveSearchResult("search:{city:" + city + "}:" +
                                  std::to_string(from) + ":" +
                                  std::to_string(to),
                              *properties);
    }
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{*properties}.ExtractValue());
  }

  if (request.HasArg("min_price") && request.HasArg("max_price")) {
    try {
      double min = std::stod(request.GetArg("min_price"));
      double max = std::stod(request.GetArg("max_price"));
      auto properties =
          cache_.SearchPripertiesByPriceRange(min, max, from, to);
      if (!properties.has_value()) {
        properties = storage_.GetPropertiesByPriceRange(min, max, to, from);
        cache_.SaveSearchResult(
            "search:price:" + std::to_string(min) + ":" + std::to_string(max) +
                ":" + std::to_string(from) + ":" + std::to_string(to),
            *properties);
      }
      return userver::formats::json::ToString(
          userver::formats::json::ValueBuilder{*properties}.ExtractValue());
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