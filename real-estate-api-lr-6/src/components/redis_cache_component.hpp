#pragma once

#include <string>
#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/redis/reply.hpp>
#include <vector>

#include "../models/dto.hpp"

namespace components {

class RedisCacheComponent final : public userver::components::ComponentBase {
public:
  static constexpr std::string_view kName = "redis-cache-component";

  RedisCacheComponent(const userver::components::ComponentConfig &config,
                      const userver::components::ComponentContext &context);

  std::optional<std::vector<models::dto::PropertyResponse>>
  SearchPropertiesByCity(const std::string &city, int offset,
                         int limit);

  void SaveSearchResult(
      const std::string &key,
      const std::vector<models::dto::PropertyResponse> &data);

  void InvalidateCityCache(const std::string &city);

  std::optional<std::vector<models::dto::PropertyResponse>>
  SearchPripertiesByPriceRange(double min_price, double max_price,
                               int offset, int limit);

  std::optional<models::dto::UserResponse>
  SearchUserByLogin(const std::string &login);

  void SaveUserLoginResult(
        const std::string &key,
        const models::dto::UserResponse &data);

private:
  std::shared_ptr<userver::storages::redis::Client> redis_client_;
};

} // namespace components