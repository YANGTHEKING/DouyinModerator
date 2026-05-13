cmake_minimum_required(VERSION 3.19)
project(DouyinModerator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.11 REQUIRED COMPONENTS Widgets Network WebSockets WebEngineWidgets)
find_package(Protobuf REQUIRED)
find_package(ZLIB REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(utf8_range CONFIG REQUIRED)

add_subdirectory(proto)
add_subdirectory(src/models)
add_subdirectory(src/core)
add_subdirectory(src/ui)

add_executable(${PROJECT_NAME} src/main.cpp)

# Link all protobuf transitive dependencies explicitly
target_link_libraries(${PROJECT_NAME} PRIVATE
    core_lib models_lib ui_lib
    Qt6::Widgets Qt6::Network Qt6::WebSockets Qt6::WebEngineWidgets
    protobuf::libprotobuf ZLIB::ZLIB
    absl::absl_check absl::absl_log absl::algorithm absl::base absl::bind_front
    absl::bits absl::btree absl::cleanup absl::cord absl::core_headers
    absl::debugging absl::die_if_null absl::dynamic_annotations absl::flags
    absl::flat_hash_map absl::flat_hash_set absl::function_ref absl::hash
    absl::layout absl::log_initialize absl::log_globals absl::log_severity
    absl::memory absl::node_hash_map absl::node_hash_set absl::optional
    absl::random_distributions absl::random_random absl::span absl::status
    absl::statusor absl::strings absl::synchronization absl::time absl::utility
    utf8_range::utf8_validity utf8_range::utf8_range)
