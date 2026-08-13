cmake_minimum_required(VERSION 3.14)
project(my_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=x86-64 -mtune=generic")

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/analysis_engine)
set(TESTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp_tests)

include(FetchContent)


FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)

FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.0
)

FetchContent_MakeAvailable(json catch2)


add_executable(sockets_test ${TESTS}/sockets_test.cpp)
target_link_libraries(sockets_test PRIVATE Catch2::Catch2WithMain)
target_link_libraries(sockets_test PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(sockets_test PRIVATE ${SRC}/)

add_executable(graph_test
    ${TESTS}/graph_test.cpp
    ${SRC}/graph/graph.cpp)
target_link_libraries(graph_test PRIVATE Catch2::Catch2WithMain)
target_link_libraries(graph_test PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(graph_test PRIVATE ${SRC}/graph/)
target_include_directories(graph_test PRIVATE ${SRC}/handle_strategies/)


add_executable(server
    ${SRC}/server.cpp
    ${SRC}/graph/graph.cpp)
target_link_libraries(server PRIVATE Catch2::Catch2WithMain)
target_link_libraries(server PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(server PRIVATE ${SRC}/graph/)
target_include_directories(server PRIVATE ${SRC}/analysis_engine)


