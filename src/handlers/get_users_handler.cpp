#include "../models/dto.hpp"
#include "get_users_handler.hpp"

#include <exception>
#include <stdexcept>
#include <string>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/server/http/http_status.hpp>

namespace handlers {

GetUsersHandler::GetUsersHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<components::PostgresStorageComponent>()),
      cache_(context.FindComponent<components::RedisCacheComponent>()) {}

std::string GetUsersHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest &request,
    userver::server::request::RequestContext &) const {

  request.GetHttpResponse().SetContentType(
      userver::http::content_type::kApplicationJson);

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

  if (request.HasArg("login")) {
    auto user = cache_.SearchUserByLogin(request.GetArg("login"));
    if (!user) {
      user = storage_.GetUserByLogin(request.GetArg("login"), from, to);
    }
    if (!user) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      models::dto::ErrorResponse error{"NOT_FOUND", "User not found"};
      return userver::formats::json::ToString(
          userver::formats::json::ValueBuilder{error}.ExtractValue());
    }
    cache_.SaveUserLoginResult("user:login:" + user->login, *user);
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{std::vector{*user}}
            .ExtractValue());
  }

  if (request.HasArg("name_mask")) {
    auto users = storage_.SearchUsersByNameMask(request.GetArg("name_mask"), from, to);
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{users}.ExtractValue());
  }

  request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
  models::dto::ErrorResponse error{
      "BAD_REQUEST", "Missing query parameter (login or name_mask)"};
  return userver::formats::json::ToString(
      userver::formats::json::ValueBuilder{error}.ExtractValue());
}

} // namespace handlers