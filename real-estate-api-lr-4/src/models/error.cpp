#include "error.hpp"

namespace models::dto {

userver::formats::json::Value
Serialize(const ErrorResponse &data,
          userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["code"] = data.code;
  builder["message"] = data.message;
  if (data.details) {
    builder["details"] = *data.details;
  }
  return builder.ExtractValue();
}

ErrorResponse
Parse(const userver::formats::json::Value &json,
      userver::formats::parse::To<ErrorResponse>) {
  ErrorResponse result;
  result.code = json["code"].As<std::string>();
  result.message = json["message"].As<std::string>();
  if (json.HasMember("details")) {
    result.details = json["details"];
  }
  return result;
}

} // namespace models::dto