#include "./components/auth_component.hpp"
#include "./components/mongo_storage_component.hpp"
#include "./components/postgres_storage_component.hpp"
#include "./components/rate_limiter_component.hpp"
#include "./jwt_auth/jwt_auth_checker.hpp"
#include "./jwt_auth/jwt_auth_factory.hpp"
#include "components/redis_cache_component.hpp"
#include "handlers/create_property_handler.hpp"
#include "handlers/create_user_handler.hpp"
#include "handlers/create_viewing_handler.hpp"
#include "handlers/get_properties_handler.hpp"
#include "handlers/get_users_handler.hpp"
#include "handlers/login_handler.hpp"
#include "handlers/openapi_handler.hpp"
#include "handlers/swagger_ui_handler.hpp"
#include <userver/components/component_list.hpp>
#include <userver/formats/json/parser/base_parser.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/handlers/ping.hpp>

#include <userver/server/component.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/server/handlers/ping.hpp>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/clients/dns/component.hpp>

#include <userver/storages/mongo/component.hpp>

#include <userver/storages/redis/component.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/subscribe_client.hpp>

#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider.hpp>
#include <userver/storages/secdist/provider_component.hpp>

#include <userver/engine/run_standalone.hpp>
#include <userver/yaml_config/yaml_config.hpp>

int main(int argc, char *argv[]) {
    userver::server::handlers::auth::RegisterAuthCheckerFactory<
        auth::jwt::JwtAuthCheckerFactory>();
    auto list =
        userver::components::MinimalServerComponentList()
            .Append<components::RateLimiterComponent>("rate-limit-component")
            .Append<userver::components::TestsuiteSupport>()
            .Append<userver::clients::dns::Component>()
            .Append<userver::components::Postgres>("db-postgresql")
            .Append<userver::components::Mongo>("mongo")
            .Append<userver::components::DefaultSecdistProvider>()
            .Append<userver::components::Secdist>()
            .Append<userver::components::Redis>("redis")
            .Append<userver::server::handlers::Ping>()
            .Append<components::PostgresStorageComponent>(
                "postgres-storage-component")
            .Append<components::MongoStorageComponent>(
                "mongo-storage-component")
            .Append<components::RedisCacheComponent>("redis-cache-component")
            .Append<components::AuthComponent>("auth-component")
            .Append<auth::jwt::JwtAuthComponent>()
            .Append<handlers::LoginHandler>()
            .Append<handlers::CreatePropertyHandler>()
            .Append<handlers::CreateUserHandler>()
            .Append<handlers::CreateViewingHandler>()
            .Append<handlers::GetUsersHandler>()
            .Append<handlers::GetPropertiesHandler>()
            .Append<handlers::OpenApiHandler>()
            .Append<handlers::SwaggerUiHandler>();

    return userver::utils::DaemonMain(argc, argv, list);
}