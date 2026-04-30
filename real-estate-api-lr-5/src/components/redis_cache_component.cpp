#include "./redis_cache_component.hpp"
#include <chrono>
#include <optional>
#include <userver/logging/log.hpp>
#include <userver/storages/redis/command_control.hpp>
#include <userver/storages/redis/command_options.hpp>

namespace components {

namespace {

constexpr int kUserTtl = 300;
constexpr int kSearchTtl = 60;

std::string MakeUserLoginKey(const std::string &login) { return "user:login:" + login; }

std::string MakeSearchByCityKey(const std::string &city, int offset, int limit) {
  return "search:{city:" + city + "}:" +
         std::to_string(offset) + ":" + std::to_string(limit);
}

std::string MakeSearchByPriceRangeKey(double min_price, double max_price, int offset, int limit) {
  return "search:price:" + std::to_string(min_price) +
         ":" + std::to_string(max_price) + ":" +
         std::to_string(offset) + ":" + std::to_string(limit);
}

} // namespace

    RedisCacheComponent::RedisCacheComponent(const userver::components::ComponentConfig &config,
                        const userver::components::ComponentContext &context)
        : ComponentBase(config, context),
          redis_client_(
              context.FindComponent<userver::components::Redis>("redis").GetClient("main")) {}

    // ==================== SEARCH CACHE ====================

    std::optional<std::vector<models::dto::PropertyResponse>>
    RedisCacheComponent::SearchPropertiesByCity(const std::string &city,
                                                int offset, int limit) {

      std::vector<models::dto::PropertyResponse> result;

      const auto key = MakeSearchByCityKey(city, offset, limit);

      auto reply = redis_client_->Get(key, userver::storages::redis::CommandControl{}).Get();

      if (!reply.has_value()) {
        return std::nullopt;
      }

      try {
        auto json = userver::formats::json::FromString(reply.value());

        for (const auto &item : json) {
          result.push_back(item.As<models::dto::PropertyResponse>());
        }

      } catch (...) {
        LOG_WARNING() << "Failed to parse cached city search";
      }

      return result;
    }

    void RedisCacheComponent::InvalidateCityCache(const std::string &city) {
      const auto pattern = "search:{city:" + city + "}*";

      std::vector<std::string> keys_to_delete;
      size_t cursor = 0;

      do {
        auto scan_result =
            redis_client_
                ->Scan(cursor,
                       userver::storages::redis::ScanOptionsGeneric{
                           userver::storages::redis::Match{pattern},
                           userver::storages::redis::Count{100}
                       },
                       userver::storages::redis::CommandControl{})
                .GetAll();

        for (const auto &key : scan_result) {
          keys_to_delete.push_back(key);
        }
      } while (cursor != 0);

      if (!keys_to_delete.empty()) {
        const size_t batch_size = 1000;
        for (size_t i = 0; i < keys_to_delete.size(); i += batch_size) {
          auto end = std::min(i + batch_size, keys_to_delete.size());
          std::vector<std::string> batch(keys_to_delete.begin() + i,
                                         keys_to_delete.begin() + end);

          redis_client_->Del(batch, userver::storages::redis::CommandControl{}).Get();
        }
      }
    }

    std::optional<std::vector<models::dto::PropertyResponse>>
    RedisCacheComponent::SearchPripertiesByPriceRange(double min_price,
                                                      double max_price,
                                                      int offset, int limit) {

      std::vector<models::dto::PropertyResponse> result{};

      const auto key =
          MakeSearchByPriceRangeKey(min_price, max_price, offset, limit);

      auto reply = redis_client_->Get(key, userver::storages::redis::CommandControl{}).Get();

      if (!reply.has_value()) {
        return std::nullopt;
      }

      try {
        auto json = userver::formats::json::FromString(reply.value());

        for (const auto &item : json) {
          result.push_back(item.As<models::dto::PropertyResponse>());
        }

      } catch (...) {
        LOG_WARNING() << "Failed to parse cached price search";
      }

      return result;
    }

    // ==================== SAVE SEARCH ====================

    void RedisCacheComponent::SaveSearchResult(
        const std::string &key,
        const std::vector<models::dto::PropertyResponse> &data) {

      userver::formats::json::ValueBuilder builder;

      for (const auto &item : data) {
        builder.PushBack(
            userver::formats::json::ValueBuilder(item).ExtractValue());
      }

      redis_client_->Setex(key, std::chrono::seconds(kSearchTtl),
                   userver::formats::json::ToString(builder.ExtractValue()),
                   userver::storages::redis::CommandControl{}).Get();
    }

    // ==================== USER CACHE ====================

    std::optional<models::dto::UserResponse>
    RedisCacheComponent::SearchUserByLogin(const std::string &login) {
      models::dto::UserResponse result{};

      try {
        auto reply = redis_client_->Get(MakeUserLoginKey(login), userver::storages::redis::CommandControl{}).Get();

        if (!reply.has_value()) {
          return std::nullopt;
        }

        auto json = userver::formats::json::FromString(reply.value());
        result = json.As<models::dto::UserResponse>();

      } catch (...) {
        LOG_WARNING() << "User not found in cache";
      }

      return result;
    }

    void RedisCacheComponent::SaveUserLoginResult(
        const std::string &key,
        const models::dto::UserResponse &data) {

      userver::formats::json::ValueBuilder builder{
          userver::formats::json::ValueBuilder(data).ExtractValue()};

      redis_client_
          ->Setex(key, std::chrono::seconds(kUserTtl),
                  userver::formats::json::ToString(builder.ExtractValue()),
                  userver::storages::redis::CommandControl{})
          .Get();
    }

} // namespace components