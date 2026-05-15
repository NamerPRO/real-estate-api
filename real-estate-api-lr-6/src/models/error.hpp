#pragma once

#include <optional>
#include <string>
#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>

namespace models::dto {

struct ErrorResponse {
  std::string code;
  std::string message;
  std::optional<userver::formats::json::Value> details;
};

userver::formats::json::Value
Serialize(const ErrorResponse &data,
          userver::formats::serialize::To<userver::formats::json::Value>);

ErrorResponse
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<ErrorResponse>);

} // namespace models::dto