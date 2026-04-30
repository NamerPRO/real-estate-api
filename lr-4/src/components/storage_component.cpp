#include "storage_component.hpp"
#include <cstdint>
#include <exception>
#include <string>
#include <userver/logging/log.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/storages/postgres/io/row_types.hpp>
#include <userver/storages/postgres/postgres.hpp>
#include <userver/storages/postgres/row.hpp>
#include <userver/storages/postgres/sql_state.hpp>

auto handleError =
    [](const std::string &msg, int code) {
      LOG_WARNING() << msg;
      return code;
    };

namespace components {

  using userver::storages::postgres::ClusterHostType;

  StorageComponent::StorageComponent(
      const userver::components::ComponentConfig &config,
      const userver::components::ComponentContext &context)
      : ComponentBase(config, context),
        cluster_(
            context
                .FindComponent<userver::components::Postgres>("db-postgresql")
                .GetCluster()) {}

  // ==================== USER OPERATIONS ====================

  int64_t StorageComponent::RegisterUser(
      const models::dto::UserCreateRequest &request,
      const std::string &password_hash) {

    const std::string query = R"(
    INSERT INTO users (login, password_hash, first_name, last_name, email, created_at)
    VALUES ($1, $2, $3, $4, $5, NOW())
    RETURNING id
  )";

    try {
      auto result = cluster_->Execute(
          ClusterHostType::kMaster, query, request.login, password_hash,
          request.first_name, request.last_name, request.email);
      return result.AsSingleRow<int64_t>();
    } catch (const userver::storages::postgres::UniqueViolation &e) {
      return handleError("Registration failed: user with login '" + request.login +
                      "' and/or email '" + request.email + "' already exists.",
                  uniqueViolation);
    } catch (const userver::storages::postgres::CheckViolation &e) {
      return handleError("Registration failed: data constraints were not matched.",
                  constraintViolation);
    } catch (const userver::storages::postgres::DataException &e) {
      return handleError(
          "Registration failed: data constraints were not matched.",
          constraintViolation);
    }
  }

  std::optional<int64_t> StorageComponent::VerifyCredentials(
      const std::string &login, const std::string &password_plain) {

    struct UserAuthData {
      int64_t id;
      std::string password_hash;
    };

    const std::string query =
        "SELECT id, password_hash FROM users WHERE login = $1";

    auto opt_row = cluster_->Execute(ClusterHostType::kMaster, query, login)
                       .AsOptionalSingleRow<UserAuthData>(
                           userver::storages::postgres::kRowTag);

    if (!opt_row) {
      return std::nullopt;
    }

    const auto &data = opt_row.value();

    if (data.password_hash == password_plain) {
      return data.id;
    }

    return std::nullopt;
  }

  std::optional<models::dto::UserResponse> StorageComponent::GetUserByLogin(
      const std::string &login, int from, int to) {
    const std::string query = R"(
    SELECT id, login, first_name, last_name, email, created_at 
    FROM users 
    WHERE login = $1
    LIMIT $2
    OFFSET $3
  )";

    auto opt_row = cluster_
                       ->Execute(ClusterHostType::kMaster, query, login,
                                 to - from + 1, from - 1)
                       .AsOptionalSingleRow<models::dto::UserResponse>(
                           userver::storages::postgres::kRowTag);

    if (!opt_row) {
      return std::nullopt;
    }

    return opt_row.value();
  }

  int64_t StorageComponent::CreateUser(
      const models::dto::UserCreateRequest &request,
      const std::string &password_hash) {
    return RegisterUser(request, password_hash);
  }

  std::optional<models::dto::UserResponse> StorageComponent::GetUserById(
      int64_t id) {
    const std::string query = R"(
    SELECT id, login, first_name, last_name, email, created_at 
    FROM users 
    WHERE id = $1
  )";

    auto opt_row = cluster_->Execute(ClusterHostType::kMaster, query, id)
                       .AsOptionalSingleRow<models::dto::UserResponse>(
                           userver::storages::postgres::kRowTag);

    if (!opt_row) {
      return std::nullopt;
    }

    return opt_row.value();
  }

  std::vector<models::dto::UserResponse>
  StorageComponent::SearchUsersByNameMask(const std::string &mask, int from,
                                          int to) {
    std::string search_pattern = mask;
    std::replace(search_pattern.begin(), search_pattern.end(), '*', '%');

    const std::string query = R"(
    SELECT id, login, first_name, last_name, email, created_at 
    FROM users 
    WHERE (first_name || ' ' || last_name) LIKE $1
    LIMIT $2
    OFFSET $3
  )";

    auto rows = cluster_->Execute(ClusterHostType::kMaster, query,
                                  search_pattern, to - from + 1, from - 1);

    std::vector<models::dto::UserResponse> result;
    result.reserve(rows.Size());

    for (const auto &row : rows) {
      result.push_back(
          {row["id"].As<int64_t>(), row["login"].As<std::string>(),
           row["first_name"].As<std::string>(),
           row["last_name"].As<std::string>(), row["email"].As<std::string>(),
           row["created_at"].As<userver::storages::postgres::TimePointTz>()});
    }

    return result;
  }

  // ==================== PROPERTY OPERATIONS ====================

  int64_t StorageComponent::CreateProperty(
      const models::dto::PropertyCreateRequest &request) {

    auto owner_check =
        cluster_
            ->Execute(ClusterHostType::kMaster,
                      "SELECT 1 FROM users WHERE id = $1", request.owner_id)
            .AsOptionalSingleRow<int>();

    if (!owner_check) {
      return dataViolation;
    }

    const std::string query = R"(
    INSERT INTO properties (owner_id, title, city, price, status, created_at)
    VALUES ($1, $2, $3, $4, $5, NOW())
    RETURNING id
  )";

    try {
      auto result = cluster_->Execute(
          ClusterHostType::kMaster, query, request.owner_id, request.title,
          request.city, request.price, request.status);
      return result.AsSingleRow<int64_t>();
    } catch (const userver::storages::postgres::CheckViolation &e) {
      return handleError(
          "Failed to create property: data constraints were not matched.",
          constraintViolation);
    } catch (const userver::storages::postgres::DataException &e) {
      return handleError(
          "Failed to create property: data constraints were not matched.",
          constraintViolation);
    } catch (const std::exception &e) {
      LOG_ERROR() << "Failed to create property: " << e.what();
      throw;
    }
  }

  std::optional<models::dto::PropertyResponse>
  StorageComponent::GetPropertyById(int64_t id) {
    const std::string query = R"(
    SELECT id, owner_id, title, city, price, status, created_at 
    FROM properties 
    WHERE id = $1
  )";

    auto row_opt = cluster_->Execute(ClusterHostType::kMaster, query, id)
                       .AsOptionalSingleRow<models::dto::PropertyResponse>(
                           userver::storages::postgres::kRowTag);

    if (!row_opt)
      return std::nullopt;

    return row_opt.value();
  }

  std::vector<models::dto::PropertyResponse>
  StorageComponent::GetPropertiesByCity(const std::string &city, int from,
                                        int to) {
    const std::string query = R"(
    SELECT id, owner_id, title, city, price, status, created_at 
    FROM properties 
    WHERE city = $1
    LIMIT $2
    OFFSET $3
  )";

    auto rows = cluster_->Execute(ClusterHostType::kMaster, query, city,
                                  to - from + 1, from - 1);
    std::vector<models::dto::PropertyResponse> result;
    result.reserve(rows.Size());

    for (const auto &row : rows) {
      result.push_back(
          {row["id"].As<int64_t>(), row["owner_id"].As<int64_t>(),
           row["title"].As<std::string>(), row["city"].As<std::string>(),
           row["price"].As<double>(), row["status"].As<std::string>(),
           row["created_at"].As<userver::storages::postgres::TimePointTz>()});
    }
    return result;
  }

  std::vector<models::dto::PropertyResponse>
  StorageComponent::GetPropertiesByPriceRange(double min, double max, int from,
                                              int to) {
    const std::string query = R"(
    SELECT id, owner_id, title, city, price, status, created_at 
    FROM properties 
    WHERE price >= $1 AND price <= $2
    LIMIT $3
    OFFSET $4
  )";

    auto rows = cluster_->Execute(ClusterHostType::kMaster, query, min, max,
                                  to - from + 1, from - 1);
    std::vector<models::dto::PropertyResponse> result;
    result.reserve(rows.Size());

    for (const auto &row : rows) {
      result.push_back(
          {row["id"].As<int64_t>(), row["owner_id"].As<int64_t>(),
           row["title"].As<std::string>(), row["city"].As<std::string>(),
           row["price"].As<double>(), row["status"].As<std::string>(),
           row["created_at"].As<userver::storages::postgres::TimePointTz>()});
    }
    return result;
  }

  std::vector<models::dto::PropertyResponse>
  StorageComponent::GetUserProperties(int64_t user_id) {
    const std::string query = R"(
    SELECT id, owner_id, title, city, price, status, created_at 
    FROM properties 
    WHERE owner_id = $1
  )";

    auto rows = cluster_->Execute(ClusterHostType::kMaster, query, user_id);
    std::vector<models::dto::PropertyResponse> result;
    result.reserve(rows.Size());

    for (const auto &row : rows) {
      result.push_back(
          {row["id"].As<int64_t>(), row["owner_id"].As<int64_t>(),
           row["title"].As<std::string>(), row["city"].As<std::string>(),
           row["price"].As<double>(), row["status"].As<std::string>(),
           row["created_at"].As<userver::storages::postgres::TimePointTz>()});
    }
    return result;
  }

  bool StorageComponent::UpdatePropertyStatus(int64_t id,
                                              const std::string &status) {
    const std::string query = R"(
    UPDATE properties 
    SET status = $1 
    WHERE id = $2
  )";
    auto result =
        cluster_->Execute(ClusterHostType::kMaster, query, status, id);
    return result.RowsAffected() > 0;
  }

  // ==================== VIEWING OPERATIONS ====================
  int64_t StorageComponent::CreateViewing(
      const models::dto::ViewingCreateRequest &request) {

    auto prop_check = cluster_
                          ->Execute(ClusterHostType::kMaster,
                                    "SELECT 1 FROM properties WHERE id = $1",
                                    request.property_id)
                          .AsOptionalSingleRow<int>();

    if (!prop_check) {
      return dataViolation;
    }

    const std::string query = R"(
    INSERT INTO viewings (property_id, user_id, scheduled_time, status, comment, created_at)
    VALUES ($1, $2, $3, $4, $5, NOW())
    RETURNING id
  )";

    try {
      auto result = cluster_->Execute(ClusterHostType::kMaster, query,
                                      request.property_id, request.user_id,
                                      request.scheduled_time, "scheduled",
                                      request.comment);
      return result.AsSingleRow<int64_t>();
    } catch (const userver::storages::postgres::CheckViolation &e) {
      return handleError(
          "Failed to create property: data constraints were not matched.",
          constraintViolation);
    } catch(const userver::storages::postgres::DataException &e) {
      return handleError(
          "Failed to create property: data constraints were not matched.",
          constraintViolation);
    } catch (const std::exception &e) {
      LOG_ERROR() << "Failed to create viewing: " << e.what();
      throw;
    }
  }

  std::vector<models::dto::ViewingResponse>
  StorageComponent::GetPropertyViewings(int64_t property_id) {
    const std::string query = R"(
    SELECT id, property_id, user_id, scheduled_time, status, created_at 
    FROM viewings 
    WHERE property_id = $1
  )";

    auto rows = cluster_->Execute(ClusterHostType::kMaster, query, property_id);
    std::vector<models::dto::ViewingResponse> result;
    result.reserve(rows.Size());

    for (const auto &row : rows) {
      result.push_back(
          {row["id"].As<int64_t>(), row["property_id"].As<int64_t>(),
           row["user_id"].As<int64_t>(),
           row["scheduled_time"].As<userver::storages::postgres::TimePointTz>(),
           row["status"].As<std::string>(),
           row["created_at"].As<userver::storages::postgres::TimePointTz>()});
    }
    return result;
  }

  std::optional<models::dto::ViewingResponse>
  StorageComponent::GetViewingById(int64_t id) {
    const std::string query = R"(
    SELECT id, property_id, user_id, scheduled_time, status, created_at 
    FROM viewings
    WHERE id = $1
  )";

    auto row_opt = cluster_->Execute(ClusterHostType::kMaster, query, id)
                       .AsOptionalSingleRow<models::dto::ViewingResponse>(
                           userver::storages::postgres::kRowTag);

    if (!row_opt)
      return std::nullopt;

    return row_opt.value();
  }

} // namespace components