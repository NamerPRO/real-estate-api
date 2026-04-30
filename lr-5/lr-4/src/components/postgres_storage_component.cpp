#include "postgres_storage_component.hpp"
#include <cstdint>
#include <string>
#include <userver/logging/log.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/storages/postgres/io/row_types.hpp>
#include <userver/storages/postgres/postgres.hpp>
#include <userver/storages/postgres/row.hpp>
#include <userver/storages/postgres/sql_state.hpp>

auto handlePostgresError =
    [](const std::string &msg, int code) {
      LOG_WARNING() << msg;
      return code;
    };

namespace components {

  using userver::storages::postgres::ClusterHostType;

  PostgresStorageComponent::PostgresStorageComponent(
      const userver::components::ComponentConfig &config,
      const userver::components::ComponentContext &context)
      : ComponentBase(config, context),
        cluster_(
            context
                .FindComponent<userver::components::Postgres>("db-postgresql")
                .GetCluster()) {}

  // ==================== USER OPERATIONS ====================

  int64_t PostgresStorageComponent::RegisterUser(
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
      return handlePostgresError("Registration failed: user with login '" + request.login +
                      "' and/or email '" + request.email + "' already exists.",
                  uniqueViolation);
    } catch (const userver::storages::postgres::CheckViolation &e) {
      return handlePostgresError("Registration failed: data constraints were not matched.",
                  constraintViolation);
    } catch (const userver::storages::postgres::DataException &e) {
      return handlePostgresError(
          "Registration failed: data constraints were not matched.",
          constraintViolation);
    }
  }

  std::optional<int64_t> PostgresStorageComponent::VerifyCredentials(
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

  std::optional<models::dto::UserResponse> PostgresStorageComponent::GetUserByLogin(
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

  int64_t PostgresStorageComponent::CreateUser(
      const models::dto::UserCreateRequest &request,
      const std::string &password_hash) {
    return RegisterUser(request, password_hash);
  }

  std::optional<models::dto::UserResponse> PostgresStorageComponent::GetUserById(
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
  PostgresStorageComponent::SearchUsersByNameMask(const std::string &mask, int from,
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

} // namespace components