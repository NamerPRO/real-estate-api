#include "mongo_storage_component.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <userver/formats/bson.hpp>
#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/inline.hpp>
#include <userver/formats/bson/iterator.hpp>
#include <userver/formats/bson/serialize.hpp>
#include <userver/formats/bson/types.hpp>
#include <userver/formats/bson/value.hpp>
#include <userver/formats/bson/value_builder.hpp>
#include <userver/formats/common/conversion_stack.hpp>
#include <userver/formats/common/utils.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/storages/mongo/options.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/uuid4.hpp>

bool IsDuplicateKeyError(const std::exception &e) {
  const std::string msg = e.what();
  return msg.find("E11000") != std::string::npos ||
         msg.find("duplicate key") != std::string::npos;
}

auto handleMongoError = [](const std::string &msg, const std::string &code) {
  LOG_WARNING() << msg;
  return code;
};

namespace components {

// ==================== CONSTRUCTOR ====================

MongoStorageComponent::MongoStorageComponent(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : ComponentBase(config, context),
      mongo_pool_(
          context.FindComponent<userver::components::Mongo>("mongo").GetPool()),
      properties_collection_(mongo_pool_->GetCollection("properties")),
      viewings_collection_(mongo_pool_->GetCollection("viewings")) {}

// ==================== PROPERTY OPERATIONS ====================

std::string MongoStorageComponent::CreateProperty(
    const models::dto::PropertyCreateRequest &request) {

  try {
    auto doc = MapPropertyToBson(request);
    properties_collection_.InsertOne(doc);
    return doc["_id"].As<std::string>();

  } catch (const std::exception &e) {
    if (IsDuplicateKeyError(e)) {
      return handleMongoError("Failed to create property: duplicate key violation",
                         uniqueViolation);
    }

    return handleMongoError(
        "Failed to create property: data constraints were not matched.",
        constraintViolation);
  }
}

std::optional<models::dto::PropertyResponse>
MongoStorageComponent::GetPropertyById(const std::string &id) {

  auto doc = properties_collection_.FindOne(userver::formats::bson::MakeDoc(
      "_id", id));

  if (!doc)
    return std::nullopt;

  return MapBsonToProperty(*doc);
}

std::vector<models::dto::PropertyResponse>
MongoStorageComponent::GetPropertiesByCity(const std::string &city, int to,
                                           int from) {

  std::vector<models::dto::PropertyResponse> result;

  auto cursor = properties_collection_.Find(
      userver::formats::bson::MakeDoc("city", city),
      userver::storages::mongo::options::Limit(to - from + 1),
      userver::storages::mongo::options::Skip(from - 1));

  result.reserve(to - from + 1);

  for (const auto &doc : cursor) {
    result.push_back(MapBsonToProperty(doc));
  }

  return result;
}

std::vector<models::dto::PropertyResponse>
MongoStorageComponent::GetPropertiesByPriceRange(double min_price,
                                                 double max_price, int to,
                                                 int from) {

  std::vector<models::dto::PropertyResponse> result;

  auto cursor = properties_collection_.Find(
      userver::formats::bson::MakeDoc(
          "price", userver::formats::bson::MakeDoc("$gte", min_price, "$lte",
                                                   max_price)),
      userver::storages::mongo::options::Limit(to - from + 1),
      userver::storages::mongo::options::Skip(from - 1));

  result.reserve(to - from + 1);
  
  for (const auto &doc : cursor) {
    result.push_back(MapBsonToProperty(doc));
  }

  return result;
}

std::vector<models::dto::PropertyResponse>
MongoStorageComponent::GetUserProperties(int64_t owner_id) {

  std::vector<models::dto::PropertyResponse> result;

  auto cursor = properties_collection_.Find(
      userver::formats::bson::MakeDoc("owner_id", owner_id));

  for (const auto &doc : cursor) {
    result.push_back(MapBsonToProperty(doc));
  }

  return result;
}

bool MongoStorageComponent::UpdatePropertyStatus(const std::string &property_id,
                                                 const std::string &status) {
  try {
    auto result = properties_collection_.UpdateOne(
        userver::formats::bson::MakeDoc("_id", property_id),
        userver::formats::bson::MakeDoc(
            "$set", userver::formats::bson::MakeDoc("status", status)));

    return result.MatchedCount() > 0;

  } catch (const std::exception &e) {
    LOG_ERROR() << "UpdatePropertyStatus failed: " << e.what();
    return false;
  }
}

bool MongoStorageComponent::AddFeatureToProperty(const std::string &property_id,
                                                 const std::string &feature) {
  try {
    auto result = properties_collection_.UpdateOne(
        userver::formats::bson::MakeDoc("_id", property_id),
        userver::formats::bson::MakeDoc(
            "$addToSet", userver::formats::bson::MakeDoc("features", feature)));

    return result.MatchedCount() > 0;

  } catch (const std::exception &e) {
    LOG_ERROR() << "AddFeatureToProperty failed: " << e.what();
    return false;
  }
}

bool MongoStorageComponent::DeleteProperty(const std::string &property_id) {

  try {
    auto result = properties_collection_.DeleteOne(
        userver::formats::bson::MakeDoc("_id", property_id));

    return result.DeletedCount() > 0;

  } catch (const std::exception &e) {
    LOG_ERROR() << "DeleteProperty failed: " << e.what();
    return false;
  }
}

// ==================== VIEWING OPERATIONS ====================

std::string MongoStorageComponent::CreateViewing(
    const models::dto::ViewingCreateRequest &request) {

  try {
    auto prop = properties_collection_.FindOne(
        userver::formats::bson::MakeDoc("_id", request.property_id));

    if (!prop) {
      return handleMongoError("Property not found", dataViolation);
    }

    auto doc = MapViewingToBson(request);
    viewings_collection_.InsertOne(doc);
    return doc["_id"].As<std::string>();

  } catch (const std::exception &e) {

    if (IsDuplicateKeyError(e)) {
      return handleMongoError("Failed to create viewing: duplicate key violation",
                         uniqueViolation);
    }

    return handleMongoError(
        "Failed to create viewing: data constraints were not matched.",
        constraintViolation);
  }
}

std::vector<models::dto::ViewingResponse>
MongoStorageComponent::GetPropertyViewings(const std::string &property_id) {

  std::vector<models::dto::ViewingResponse> result;

  auto cursor = viewings_collection_.Find(userver::formats::bson::MakeDoc(
      "property_id", property_id));

  for (const auto &doc : cursor) {
    result.push_back(MapBsonToViewing(doc));
  }

  return result;
}

std::optional<models::dto::ViewingResponse>
MongoStorageComponent::GetViewingById(const std::string &viewing_id) {

  auto doc = viewings_collection_.FindOne(
      userver::formats::bson::MakeDoc("_id", viewing_id));

  if (!doc)
    return std::nullopt;

  return MapBsonToViewing(*doc);
}

bool MongoStorageComponent::AddCommentToViewing(const std::string &viewing_id,
                                                const std::string &text,
                                                const std::string &author) {

  auto result = viewings_collection_.UpdateOne(
      userver::formats::bson::MakeDoc("_id", viewing_id),
      userver::formats::bson::MakeDoc(
          "$push",
          userver::formats::bson::MakeDoc(
              "comments", userver::formats::bson::MakeDoc(
                              "text", text, "author", author, "created_at",
                              userver::utils::datetime::Now()))));

  return result.MatchedCount() > 0;
}

bool MongoStorageComponent::RescheduleViewing(const std::string &viewing_id,
                                              const std::string &new_time_iso,
                                              const std::string &reason,
                                              const std::string &author) {

  try {
    auto update1 = viewings_collection_.UpdateOne(
        userver::formats::bson::MakeDoc("_id", viewing_id),
        userver::formats::bson::MakeDoc(
            "$set",
            userver::formats::bson::MakeDoc("scheduled_time", new_time_iso)));

    auto update2 = viewings_collection_.UpdateOne(
        userver::formats::bson::MakeDoc("_id", viewing_id),
        userver::formats::bson::MakeDoc(
            "$push",
            userver::formats::bson::MakeDoc(
                "history", userver::formats::bson::MakeDoc(
                               "reason", reason, "author", author, "created_at",
                               userver::utils::datetime::Now()))));

    return update1.MatchedCount() > 0 && update2.MatchedCount() > 0;

  } catch (const std::exception &e) {
    LOG_ERROR() << "RescheduleViewing failed: " << e.what();
    return false;
  }
}

// ==================== MAPPING ====================

userver::formats::bson::Value MongoStorageComponent::MapPropertyToBson(
    const models::dto::PropertyCreateRequest &req) {

  const auto id = userver::utils::generators::GenerateUuid();

  return userver::formats::bson::MakeDoc(
      "_id", id, "owner_id", req.owner_id, "type", req.type, "title", req.title,
      "city", req.city, "price", req.price, "status", req.status, "address",
      req.address.As<userver::formats::bson::Value>(), "details",
      req.details.As<userver::formats::bson::Value>(), "features",
      req.features.As<userver::formats::bson::Value>(),
      "created_at", std::chrono::system_clock::now());
}

models::dto::PropertyResponse MongoStorageComponent::MapBsonToProperty(
    const userver::formats::bson::Value &doc) {

  models::dto::PropertyResponse res;

  res.id = doc["_id"].As<std::string>();
  res.owner_id = doc["owner_id"].As<int64_t>();
  res.type = doc["type"].As<std::string>();
  res.title = doc["title"].As<std::string>();
  res.city = doc["city"].As<std::string>();
  res.price = doc["price"].As<double>();
  res.status = doc["status"].As<std::string>();

  res.address = doc["address"].As<userver::formats::json::Value>();
  res.details = doc["details"].As<userver::formats::json::Value>();
  res.features = doc["features"].As<userver::formats::json::Value>();

  res.created_at = doc["created_at"].As<std::chrono::system_clock::time_point>();

  return res;
}

userver::formats::bson::Value MongoStorageComponent::MapViewingToBson(
    const models::dto::ViewingCreateRequest &req) {

  const auto id = userver::utils::generators::GenerateUuid();

  auto comments_array = userver::formats::bson::ValueBuilder{};
  for (const auto& comment : req.comments) {
    comments_array.PushBack(userver::formats::bson::MakeDoc(
        "text", comment.text, "author", comment.author, "timestamp", comment.timestamp));
  }

  return userver::formats::bson::MakeDoc(
      "_id", id, "property_id", req.property_id, "user_id", req.user_id, "scheduled_time",
      req.scheduled_time, "status", "pending", "comments", comments_array.ExtractValue(),
      "created_at", std::chrono::system_clock::now());
}

models::dto::ViewingResponse MongoStorageComponent::MapBsonToViewing(
    const userver::formats::bson::Value &doc) {

  models::dto::ViewingResponse res;

  res.id = doc["_id"].As<std::string>();
  res.property_id = doc["property_id"].As<std::string>();
  res.user_id = doc["user_id"].As<int64_t>();
  res.scheduled_time = doc["scheduled_time"].As<std::chrono::system_clock::time_point>();
  res.status = doc["status"].As<std::string>();
  
  for (const auto &comment_bson : doc["comments"]) {
    res.comments.push_back({
      comment_bson["text"].As<std::string>(),
      comment_bson["author"].As<std::string>(),
      comment_bson["timestamp"].As<std::chrono::system_clock::time_point>()
    });
  }

  res.created_at = doc["created_at"].As<std::chrono::system_clock::time_point>();

  return res;
}

std::chrono::system_clock::time_point parse_iso8601(const std::string &s) {
  std::tm tm = {};
  std::istringstream ss(s);

  ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
  if (ss.fail())
    throw std::runtime_error("Parse error");

  std::time_t tt = timegm(&tm);
  return std::chrono::system_clock::from_time_t(tt);
}

} // namespace components