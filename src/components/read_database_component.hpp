#pragma once

#include <algorithm>
#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>

#include "../models/dto.hpp"

namespace components {

class ReadDatabaseComponent : public userver::components::ComponentBase {
public:
  inline static constexpr const char *uniqueViolation = "uniqueViolation";
  inline static constexpr const char *constraintViolation =
      "constraintViolation";
  inline static constexpr const char *dataViolation = "dataViolation";

  constexpr static int64_t pgUniqueViolation = -1;
  constexpr static int64_t pgConstraintViolation = -2;

  static constexpr std::string_view kName = "read-database-component";

  explicit ReadDatabaseComponent(
      const userver::components::ComponentConfig &config,
      const userver::components::ComponentContext &context);

  // ==================== PROPERTY OPERATIONS ====================
  std::string CreateProperty(const std::string &id, const models::dto::PropertyCreateRequest &request);
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

  // ==================== VIEWING OPERATIONS ====================
  std::string CreateViewing(const models::dto::ViewingCreateRequest &request);
  std::vector<models::dto::ViewingResponse>
  GetPropertyViewings(const std::string &property_id);
  std::optional<models::dto::ViewingResponse>
  GetViewingById(const std::string &viewing_id);
  bool AddCommentToViewing(const std::string &viewing_id,
                           const std::string &text, const std::string &author,
                           const std::chrono::system_clock::time_point &tp);

  // ==================== USER OPERATIONS ====================
  int64_t RegisterUser(const models::dto::UserCreateRequest &request,
                       const std::string &password_hash);
  std::optional<int64_t> VerifyCredentials(const std::string &login,
                                           const std::string &password_plain);
  std::optional<models::dto::UserResponse>
  GetUserByLogin(const std::string &login, int from, int to);
  int64_t CreateUser(const models::dto::UserCreateRequest &request,
                     const std::string &password_hash);
  std::optional<models::dto::UserResponse> GetUserById(int64_t id);
  std::vector<models::dto::UserResponse>
  SearchUsersByNameMask(const std::string &mask, int from, int to);

private:
  bool MatchesSimplePattern(const std::string &text, const std::string &pattern);

  struct StoredProperty {
    models::dto::PropertyResponse data;
    std::vector<std::string> features;
  };

  struct StoredViewing {
    models::dto::ViewingResponse data;
    std::vector<models::dto::Comment>
        comments;
  };

  struct StoredUser {
    models::dto::UserResponse data;
    std::string password_hash;
  };

  std::unordered_map<std::string, StoredProperty> properties_;
  std::unordered_map<std::string, StoredViewing> viewings_;
  std::unordered_map<int64_t, StoredUser> users_;
  std::unordered_map<std::string, int64_t> user_login_index_;

  mutable std::mutex mutex_;

  std::string GenerateId();
  int64_t GenerateUserId();

  template <typename T>
  std::vector<T> ApplyPagination(const std::vector<T> &items, int from,
                                 int to) const;
};

} // namespace components