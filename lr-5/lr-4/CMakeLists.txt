cmake_minimum_required(VERSION 3.10)
project(a.out VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)
enable_testing()

include(FetchContent)
FetchContent_Declare(
    jwt-cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG v0.7.2
)
FetchContent_MakeAvailable(jwt-cpp)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(USERVER_COMPONENTS core grpc postgresql mongo)
find_package(userver REQUIRED COMPONENTS ${USERVER_COMPONENTS})

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.hpp")

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PRIVATE include)

target_link_libraries(${PROJECT_NAME} PRIVATE userver::core userver::postgresql userver::mongo jwt-cpp::jwt-cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${USERVER_INCLUDE_DIRS})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)