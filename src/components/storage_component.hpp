#pragma once

#include <optional>
#include <string>
#include <vector>
#include <algorithm>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/io/chrono.hpp>

#include "../models/dto.hpp"

namespace components {

class StorageComponent : public userver::components::ComponentBase {
public:
  constexpr static int64_t uniqueViolation = -1;
  constexpr static int64_t constraintViolation = -2;
  constexpr static int64_t dataViolation = -1;

  static constexpr std::string_view kName = "storage-component";

  explicit StorageComponent(
      const userver::components::ComponentConfig &config,
      const userver::components::ComponentContext &context);

  // ==================== USER OPERATIONS ====================
  int64_t RegisterUser(const models::dto::UserCreateRequest &request,
                       const std::string &password_hash);
  std::optional<int64_t> VerifyCredentials(const std::string &login,
                                           const std::string &password_plain);
  std::optional<models::dto::UserResponse>
  GetUserByLogin(const std::string &login, int from, int to);
  int64_t CreateUser(const models::dto::UserCreateRequest &request, const std::string &password_hash);
  std::optional<models::dto::UserResponse> GetUserById(int64_t id);
  std::vector<models::dto::UserResponse>
  SearchUsersByNameMask(const std::string &mask, int from, int to);

  // ==================== PROPERTY OPERATIONS ====================
  int64_t CreateProperty(const models::dto::PropertyCreateRequest &request);
  std::optional<models::dto::PropertyResponse> GetPropertyById(int64_t id);
  std::vector<models::dto::PropertyResponse>
  GetPropertiesByCity(const std::string &city, int from, int to);
  std::vector<models::dto::PropertyResponse>
  GetPropertiesByPriceRange(double min, double max, int from, int to);
  std::vector<models::dto::PropertyResponse> GetUserProperties(int64_t user_id);
  bool UpdatePropertyStatus(int64_t id, const std::string &status);

  // ==================== VIEWING OPERATIONS ====================
  int64_t CreateViewing(const models::dto::ViewingCreateRequest &request);
  std::vector<models::dto::ViewingResponse>
  GetPropertyViewings(int64_t property_id);
  std::optional<models::dto::ViewingResponse> GetViewingById(int64_t id);

private:
  userver::storages::postgres::ClusterPtr cluster_;
};

} // namespace components