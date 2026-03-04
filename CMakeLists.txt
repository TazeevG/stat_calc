cmake_minimum_required(VERSION 3.23)
project(csv_median_calculator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -pedantic -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

find_package(Boost REQUIRED COMPONENTS filesystem program_options)

find_package(spdlog REQUIRED)
find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/csv_reader.cpp
    src/statistic_calculator.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::filesystem
    Boost::program_options
    spdlog::spdlog
    Threads::Threads
    tomlplusplus::tomlplusplus
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${tomlplusplus_SOURCE_DIR}/include
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/config.toml
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

add_executable(tests
    tests/test_main.cpp
    tests/test_stats.cpp
    tests/test_config.cpp
    tests/test_csv_reader.cpp
    tests/test_integration.cpp
    src/csv_reader.cpp
    src/statistic_calculator.cpp
)

target_link_libraries(tests PRIVATE
    Catch2::Catch2WithMain
    Boost::filesystem
    Boost::program_options
    spdlog::spdlog
    Threads::Threads
    tomlplusplus::tomlplusplus
)

target_compile_definitions(tests PRIVATE UNIT_TESTING)

target_include_directories(tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${tomlplusplus_SOURCE_DIR}/include
)