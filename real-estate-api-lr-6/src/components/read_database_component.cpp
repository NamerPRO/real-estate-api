#include "read_database_component.hpp"
#include <atomic>
#include <chrono>
#include <userver/logging/log.hpp>
#include <algorithm>

namespace components {

namespace {
std::atomic<uint64_t> id_counter{1};

std::string GenerateMockId() {
  uint64_t id = id_counter.fetch_add(1);
  return "mock-id-" + std::to_string(id);
}

} // namespace

ReadDatabaseComponent::ReadDatabaseComponent(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : userver::components::ComponentBase(config, context) {
  LOG_INFO() << "ReadDatabaseComponent (In-Memory) initialized";
}

// ==================== PROPERTY OPERATIONS ====================

std::string ReadDatabaseComponent::CreateProperty(
    const std::string &id,
    const models::dto::PropertyCreateRequest &request) {
  std::lock_guard<std::mutex> lock(mutex_);

  StoredProperty prop;
  prop.data.id = id;
  prop.data.owner_id = request.owner_id;
  prop.data.type = request.type;
  prop.data.title = request.title;
  prop.data.city = request.city;
  prop.data.address = request.address;
  prop.data.details = request.details;
  prop.data.price = request.price;
  prop.data.features = request.features;
  prop.data.status = request.status;
  prop.data.created_at = std::chrono::system_clock::now();

  properties_[id] = std::move(prop);
  return id;
}

std::optional<models::dto::PropertyResponse>
ReadDatabaseComponent::GetPropertyById(const std::string &property_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = properties_.find(property_id);
  if (it != properties_.end()) {
    return it->second.data;
  }
  return std::nullopt;
}

std::vector<models::dto::PropertyResponse>
ReadDatabaseComponent::GetPropertiesByCity(const std::string &city, int to,
                                           int from) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<models::dto::PropertyResponse> result;

  for (const auto &[id, stored_prop] : properties_) {
    if (stored_prop.data.city == city) {
      result.push_back(stored_prop.data);
    }
  }

  std::sort(result.begin(), result.end(),
            [](const auto &a, const auto &b) { return a.id < b.id; });

  return ApplyPagination(result, from, to);
}

std::vector<models::dto::PropertyResponse>
ReadDatabaseComponent::GetPropertiesByPriceRange(double min_price,
                                                 double max_price, int to,
                                                 int from) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<models::dto::PropertyResponse> result;

  for (const auto &[id, stored_prop] : properties_) {
    if (stored_prop.data.price >= min_price &&
        stored_prop.data.price <= max_price) {
      result.push_back(stored_prop.data);
    }
  }

  std::sort(result.begin(), result.end(),
            [](const auto &a, const auto &b) { return a.id < b.id; });

  return ApplyPagination(result, from, to);
}

std::vector<models::dto::PropertyResponse>
ReadDatabaseComponent::GetUserProperties(int64_t owner_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<models::dto::PropertyResponse> result;

  for (const auto &[id, stored_prop] : properties_) {
    if (stored_prop.data.owner_id == owner_id) {
      result.push_back(stored_prop.data);
    }
  }
  return result;
}

bool ReadDatabaseComponent::UpdatePropertyStatus(const std::string &property_id,
                                                 const std::string &status) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = properties_.find(property_id);
  if (it != properties_.end()) {
    it->second.data.status = status;
    return true;
  }
  return false;
}

bool ReadDatabaseComponent::AddFeatureToProperty(const std::string &property_id,
                                                 const std::string &feature) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = properties_.find(property_id);
  if (it != properties_.end()) {
    auto &features = it->second.features;
    if (std::find(features.begin(), features.end(), feature) ==
        features.end()) {
      features.push_back(feature);
    }
    return true;
  }
  return false;
}

bool ReadDatabaseComponent::DeleteProperty(const std::string &property_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  return properties_.erase(property_id) > 0;
}

// ==================== VIEWING OPERATIONS ====================

std::string ReadDatabaseComponent::CreateViewing(
    const models::dto::ViewingCreateRequest &request) {
  std::lock_guard<std::mutex> lock(mutex_);

  std::string new_id = GenerateMockId();

  StoredViewing viewing;
  viewing.data.id = new_id;
  viewing.data.property_id = request.property_id;
  viewing.data.user_id = request.user_id;
  viewing.data.scheduled_time = request.scheduled_time;
  viewing.data.status = "SCHEDULED";

  viewings_[new_id] = std::move(viewing);
  return new_id;
}

std::vector<models::dto::ViewingResponse>
ReadDatabaseComponent::GetPropertyViewings(const std::string &property_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<models::dto::ViewingResponse> result;

  for (const auto &[id, stored_viewing] : viewings_) {
    if (stored_viewing.data.property_id == property_id) {
      result.push_back(stored_viewing.data);
    }
  }
  return result;
}

std::optional<models::dto::ViewingResponse>
ReadDatabaseComponent::GetViewingById(const std::string &viewing_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = viewings_.find(viewing_id);
  if (it != viewings_.end()) {
    return it->second.data;
  }
  return std::nullopt;
}

bool ReadDatabaseComponent::AddCommentToViewing(const std::string &viewing_id,
                                                const std::string &text,
                                                const std::string &author,
                                                const std::chrono::system_clock::time_point &ts) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = viewings_.find(viewing_id);
  if (it != viewings_.end()) {
    models::dto::Comment comment;
    comment.text = text;
    comment.author = author;
    comment.timestamp = ts;
    it->second.comments.push_back(comment);
    return true;
  }
  return false;
}

// ==================== USER OPERATIONS ====================

int64_t ReadDatabaseComponent::RegisterUser(
    const models::dto::UserCreateRequest &request,
    const std::string &password_hash) {

  std::lock_guard<std::mutex> lock(mutex_);

  if (user_login_index_.count(request.login) > 0) {
    return pgUniqueViolation;
  }

  int64_t new_id = GenerateUserId();

  StoredUser user;
  user.data.id = new_id;
  user.data.login = request.login;
  user.data.first_name = request.first_name;
  user.data.last_name = request.last_name;
  user.data.email = request.email;
  user.password_hash = password_hash;
  user.data.created_at = userver::storages::postgres::TimePointTz{std::chrono::system_clock::now()};

  users_[new_id] = user;
  user_login_index_[request.login] = new_id;

  return new_id;
}

std::optional<int64_t>
ReadDatabaseComponent::VerifyCredentials(const std::string &login,
                                         const std::string &password_plain) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it_login = user_login_index_.find(login);
  if (it_login == user_login_index_.end()) {
    return std::nullopt;
  }

  int64_t user_id = it_login->second;
  const auto &stored_user = users_[user_id];
  if (stored_user.password_hash == password_plain) {
    return user_id;
  }

  return std::nullopt;
}

std::optional<models::dto::UserResponse>
ReadDatabaseComponent::GetUserByLogin(const std::string &login, int from,
                                      int to) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = user_login_index_.find(login);
  if (it != user_login_index_.end()) {
    if (from == 1 && to >= 1) {
      return users_[it->second].data;
    }
  }
  return std::nullopt;
}

int64_t
ReadDatabaseComponent::CreateUser(const models::dto::UserCreateRequest &request,
                                  const std::string &password_hash) {
  return RegisterUser(request, password_hash);
}

std::optional<models::dto::UserResponse>
ReadDatabaseComponent::GetUserById(int64_t id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = users_.find(id);
  if (it != users_.end()) {
    return it->second.data;
  }
  return std::nullopt;
}

std::vector<models::dto::UserResponse>
ReadDatabaseComponent::SearchUsersByNameMask(const std::string &mask, int from,
                                             int to) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<models::dto::UserResponse> result;

  std::string mask_lower = mask;
  std::replace(mask_lower.begin(), mask_lower.end(), '*', '%');

  std::transform(mask_lower.begin(), mask_lower.end(), mask_lower.begin(), ::tolower);

  LOG_ERROR() << mask_lower;

  for (const auto &[id, stored_user] : users_) {
    std::string name_lower = stored_user.data.first_name + " " + stored_user.data.last_name;
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

    LOG_ERROR() << name_lower;

    if (MatchesSimplePattern(name_lower, mask_lower)) {
      result.push_back(stored_user.data);
    }
  }

  std::sort(result.begin(), result.end(),
            [](const auto &a, const auto &b)
            { return a.id < b.id; });

  return ApplyPagination(result, from, to);
}

bool ReadDatabaseComponent::MatchesSimplePattern(const std::string &text, const std::string &pattern)
{
  if (pattern.front() == '%' && pattern.back() == '%') {
    std::string search = pattern.substr(1, pattern.length() - 2);
    return text.find(search) != std::string::npos;
  } else if (pattern.front() == '%') {
    std::string suffix = pattern.substr(1);
    return text.length() >= suffix.length() &&
           text.substr(text.length() - suffix.length()) == suffix;
  } else if (pattern.back() == '%') {
    std::string prefix = pattern.substr(0, pattern.length() - 1);
    return text.length() >= prefix.length() &&
           text.substr(0, prefix.length()) == prefix;
  } else {
    return text == pattern;
  }
}

// ==================== HELPERS ====================

std::string ReadDatabaseComponent::GenerateId() { return GenerateMockId(); }

int64_t ReadDatabaseComponent::GenerateUserId() {
  static std::atomic<int64_t> user_id_counter{1000};
  return user_id_counter.fetch_add(1);
}

template <typename T>
std::vector<T>
ReadDatabaseComponent::ApplyPagination(const std::vector<T> &items, int from,
                                       int to) const {
  if (items.empty())
    return {};

  int start = std::max(1, from);
  int end = std::min(static_cast<int>(items.size()), to);

  if (start > end || start > static_cast<int>(items.size())) {
    return {};
  }

  return std::vector<T>(items.begin() + start - 1, items.begin() + end);
}

} // namespace components