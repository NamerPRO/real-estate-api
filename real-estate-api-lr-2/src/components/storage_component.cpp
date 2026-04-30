#include "storage_component.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <userver/logging/log.hpp>

namespace components {

StorageComponent::StorageComponent(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : ComponentBase(config, context) {}

std::string StorageComponent::GetCurrentTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
  return ss.str();
}

// ==================== USER OPERATIONS ====================

int64_t
StorageComponent::RegisterUser(const models::dto::UserCreateRequest &request,
                               const std::string &password_hash) {

  std::lock_guard<std::mutex> lock(mutex_);

  for (const auto &[id, data] : users_) {
    if (data.request.login == request.login) {
      return -1; // Conflict
    }
  }

  int64_t id = user_id_counter_++;
  users_[id] = {id,
                request,
                password_hash,
                GetCurrentTimestamp()};

  LOG_INFO() << "Registered user with id=" << id << " login=" << request.login;
  return id;
}

std::optional<int64_t>
StorageComponent::VerifyCredentials(const std::string &login,
                                    const std::string &password_plain) {

  std::lock_guard<std::mutex> lock(mutex_);

  for (const auto &[id, data] : users_) {
    if (data.request.login == login) {
      if (data.password_hash ==
          password_plain) {
        return id;
      }
      return std::nullopt;
    }
  }

  return std::nullopt;
}

std::optional<models::dto::UserResponse>
StorageComponent::GetUserByLogin(const std::string &login) {
  std::lock_guard<std::mutex> lock(mutex_);

  for (const auto &[id, data] : users_) {
    if (data.request.login == login) {
      return models::dto::UserResponse{data.id,
                                       data.request.login,
                                       data.request.first_name,
                                       data.request.last_name,
                                       data.request.email,
                                       data.created_at};
    }
  }

  return std::nullopt;
}

int64_t
StorageComponent::CreateUser(const models::dto::UserCreateRequest &request) {
  return RegisterUser(request, "");
}

std::optional<models::dto::UserResponse>
StorageComponent::GetUserById(int64_t id) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = users_.find(id);
  if (it == users_.end()) {
    return std::nullopt;
  }

  const auto &data = it->second;
  return models::dto::UserResponse{data.id,
                                   data.request.login,
                                   data.request.first_name,
                                   data.request.last_name,
                                   data.request.email,
                                   data.created_at};
}

std::vector<models::dto::UserResponse>
StorageComponent::SearchUsersByNameMask(const std::string &mask) {
  std::lock_guard<std::mutex> lock(mutex_);

  std::vector<models::dto::UserResponse> result;
  std::string search_pattern = mask;
  if (!search_pattern.empty() && search_pattern.back() == '*') {
    search_pattern.pop_back();
  }

  for (const auto &[id, data] : users_) {
    std::string full_name =
        data.request.first_name + " " + data.request.last_name;
    if (full_name.find(search_pattern) != std::string::npos) {
      result.push_back({data.id, data.request.login, data.request.first_name,
                        data.request.last_name, data.request.email,
                        data.created_at});
    }
  }

  return result;
}

// ==================== PROPERTY OPERATIONS ====================

int64_t StorageComponent::CreateProperty(
    const models::dto::PropertyCreateRequest &request) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (users_.find(request.owner_id) == users_.end()) {
    return -1;
  }
  int64_t id = property_id_counter_++;
  properties_[id] = {id, request, GetCurrentTimestamp()};
  LOG_INFO() << "Created property with id=" << id << " city=" << request.city;
  return id;
}

std::optional<models::dto::PropertyResponse>
StorageComponent::GetPropertyById(int64_t id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = properties_.find(id);
  if (it == properties_.end())
    return std::nullopt;
  const auto &data = it->second;
  return models::dto::PropertyResponse{
      data.id,           data.request.owner_id, data.request.title,
      data.request.city, data.request.price,    data.request.status,
      data.created_at};
}

std::vector<models::dto::PropertyResponse>
StorageComponent::GetPropertiesByCity(const std::string &city) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<models::dto::PropertyResponse> result;
  for (const auto &[id, data] : properties_) {
    if (data.request.city == city) {
      result.push_back({data.id, data.request.owner_id, data.request.title,
                        data.request.city, data.request.price,
                        data.request.status, data.created_at});
    }
  }
  return result;
}

std::vector<models::dto::PropertyResponse>
StorageComponent::GetPropertiesByPriceRange(double min, double max) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<models::dto::PropertyResponse> result;
  for (const auto &[id, data] : properties_) {
    if (data.request.price >= min && data.request.price <= max) {
      result.push_back({data.id, data.request.owner_id, data.request.title,
                        data.request.city, data.request.price,
                        data.request.status, data.created_at});
    }
  }
  return result;
}

std::vector<models::dto::PropertyResponse>
StorageComponent::GetUserProperties(int64_t user_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<models::dto::PropertyResponse> result;
  for (const auto &[id, data] : properties_) {
    if (data.request.owner_id == user_id) {
      result.push_back({data.id, data.request.owner_id, data.request.title,
                        data.request.city, data.request.price,
                        data.request.status, data.created_at});
    }
  }
  return result;
}

bool StorageComponent::UpdatePropertyStatus(int64_t id,
                                            const std::string &status) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = properties_.find(id);
  if (it == properties_.end())
    return false;
  it->second.request.status = status;
  LOG_INFO() << "Updated property id=" << id << " status=" << status;
  return true;
}

// ==================== VIEWING OPERATIONS ====================

int64_t StorageComponent::CreateViewing(
    const models::dto::ViewingCreateRequest &request) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (properties_.find(request.property_id) == properties_.end()) {
    return -1;
  }
  int64_t id = viewing_id_counter_++;
  viewings_[id] = {id, request, "scheduled"};
  LOG_INFO() << "Created viewing with id=" << id
             << " property_id=" << request.property_id;
  return id;
}

std::vector<models::dto::ViewingResponse>
StorageComponent::GetPropertyViewings(int64_t property_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<models::dto::ViewingResponse> result;
  for (const auto &[id, data] : viewings_) {
    if (data.request.property_id == property_id) {
      result.push_back({data.id, data.request.property_id, data.request.user_id,
                        data.request.scheduled_time, data.status});
    }
  }
  return result;
}

} // namespace components