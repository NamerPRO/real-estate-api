#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <vector>

#include "../models/dto.hpp"

namespace components {

class StorageComponent : public userver::components::ComponentBase {
public:
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
  GetUserByLogin(const std::string &login);

  int64_t CreateUser(
      const models::dto::UserCreateRequest &request);
  std::optional<models::dto::UserResponse> GetUserById(int64_t id);
  std::vector<models::dto::UserResponse>
  SearchUsersByNameMask(const std::string &mask);

  // ==================== PROPERTY OPERATIONS ====================
  int64_t CreateProperty(const models::dto::PropertyCreateRequest &request);
  std::optional<models::dto::PropertyResponse> GetPropertyById(int64_t id);
  std::vector<models::dto::PropertyResponse>
  GetPropertiesByCity(const std::string &city);
  std::vector<models::dto::PropertyResponse>
  GetPropertiesByPriceRange(double min, double max);
  std::vector<models::dto::PropertyResponse> GetUserProperties(int64_t user_id);
  bool UpdatePropertyStatus(int64_t id, const std::string &status);

  // ==================== VIEWING OPERATIONS ====================
  int64_t CreateViewing(const models::dto::ViewingCreateRequest &request);
  std::vector<models::dto::ViewingResponse>
  GetPropertyViewings(int64_t property_id);

private:
  struct UserData {
    int64_t id;
    models::dto::UserCreateRequest request;
    std::string password_hash;
    std::string created_at;
  };

  struct PropertyData {
    int64_t id;
    models::dto::PropertyCreateRequest request;
    std::string created_at;
  };

  struct ViewingData {
    int64_t id;
    models::dto::ViewingCreateRequest request;
    std::string status = "scheduled";
  };

  std::map<int64_t, UserData> users_;
  std::map<int64_t, PropertyData> properties_;
  std::map<int64_t, ViewingData> viewings_;

  std::atomic<int64_t> user_id_counter_{1};
  std::atomic<int64_t> property_id_counter_{1};
  std::atomic<int64_t> viewing_id_counter_{1};

  std::mutex mutex_;

  std::string GetCurrentTimestamp();
};

} // namespace components