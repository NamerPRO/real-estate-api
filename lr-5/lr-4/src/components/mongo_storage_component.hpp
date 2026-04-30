#pragma once

#include <optional>
#include <string>
#include <userver/formats/bson/value_builder.hpp>
#include <userver/storages/mongo/pool.hpp>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>

#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/component.hpp>

#include <userver/formats/bson.hpp>
#include <userver/formats/bson/value.hpp>

#include <userver/utils/datetime.hpp>

#include "../models/dto.hpp"

namespace components {

class MongoStorageComponent : public userver::components::ComponentBase {
public:
  inline static constexpr const char *uniqueViolation = "uniqueViolation";
  inline static constexpr const char *constraintViolation =
      "constraintViolation";
  inline static constexpr const char *dataViolation = "dataViolation";

  static constexpr std::string_view kName = "mongo-storage-component";

  explicit MongoStorageComponent(
      const userver::components::ComponentConfig &config,
      const userver::components::ComponentContext &context);

  // ==================== PROPERTY OPERATIONS (MongoDB) ====================
  std::string CreateProperty(const models::dto::PropertyCreateRequest &request);
  std::optional<models::dto::PropertyResponse>
  GetPropertyById(const std::string &property_id);
  std::vector<models::dto::PropertyResponse>
  GetPropertiesByCity(const std::string &city, int to, int from);
  std::vector<models::dto::PropertyResponse>
  GetPropertiesByPriceRange(double min_price, double max_price, int to,
                            int from);
  std::vector<models::dto::PropertyResponse>
  GetUserProperties(int64_t owner_id);
  bool UpdatePropertyStatus(const std::string &property_id,
                            const std::string &status);
  bool AddFeatureToProperty(const std::string &property_id,
                            const std::string &feature);
  bool DeleteProperty(const std::string &property_id);

  // ==================== VIEWING OPERATIONS (MongoDB) ====================
  std::string CreateViewing(const models::dto::ViewingCreateRequest &request);
  std::vector<models::dto::ViewingResponse>
  GetPropertyViewings(const std::string &property_id);
  std::optional<models::dto::ViewingResponse>
  GetViewingById(const std::string &viewing_id);
  bool AddCommentToViewing(const std::string &viewing_id,
                           const std::string &text, const std::string &author);
  bool RescheduleViewing(const std::string &viewing_id,
                         const std::string &new_time_iso,
                         const std::string &reason,
                         const std::string &author);

private:
  userver::storages::mongo::PoolPtr mongo_pool_;

  userver::storages::mongo::Collection properties_collection_;
  userver::storages::mongo::Collection viewings_collection_;

  userver::formats::bson::Value
  MapPropertyToBson(const models::dto::PropertyCreateRequest &req);

  models::dto::PropertyResponse
  MapBsonToProperty(const userver::formats::bson::Value &doc);

  userver::formats::bson::Value
  MapViewingToBson(const models::dto::ViewingCreateRequest &req);

  models::dto::ViewingResponse
  MapBsonToViewing(const userver::formats::bson::Value &doc);

  std::chrono::system_clock::time_point
  parse_iso8601(const std::string &s);
};

} // namespace components