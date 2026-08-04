#define CATCH_CONFIG_MAIN
#include "graph.h"
#include <catch2/catch_all.hpp>
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <thread>

TEST_CASE("Graph") {
  SECTION("default") {
    EthGraph graph;
    REQUIRE_NOTHROW(graph.add_node("1", true, 0, 0));
    REQUIRE_NOTHROW(graph.add_node("2", true, 0, 0));
    REQUIRE(graph.size() == 2);
    REQUIRE_NOTHROW(graph.add_edge("1", "2", 0, 0, 0, 0));
    REQUIRE(graph.size() == 2);
  }
  SECTION("is_cycled(1)") {
    EthGraph graph;
    graph.add_node("1", true, 0, 0);
    graph.add_node("2", true, 0, 0);
    graph.add_node("3", true, 0, 0);
    graph.add_node("4", true, 0, 0);
    graph.add_node("5", true, 0, 0);
    graph.add_edge("1", "2", 0, 0, 0, 0);
    REQUIRE(graph.is_cycled("1") == false);

    graph.add_edge("2", "3", 0, 0, 0, 0);
    graph.add_edge("3", "4", 0, 0, 0, 0);
    graph.add_edge("4", "5", 0, 0, 0, 0);
    graph.add_edge("5", "1", 0, 0, 0, 0);
    REQUIRE(graph.is_cycled("1") == true);
  }
  SECTION("is_path(1)") {
    EthGraph graph;
    graph.add_node("1", true, 0, 0);
    graph.add_node("2", true, 0, 0);
    graph.add_node("3", true, 0, 0);
    graph.add_node("4", true, 0, 0);
    graph.add_node("5", true, 0, 0);
    graph.add_edge("1", "2", 0, 0, 0, 0);
    REQUIRE(graph.is_path("1", "2"));
    graph.add_edge("2", "3", 0, 0, 0, 0);
    graph.add_edge("3", "4", 0, 0, 0, 0);
    graph.add_edge("4", "5", 0, 0, 0, 0);
    graph.add_edge("5", "1", 0, 0, 0, 0);
    REQUIRE(graph.is_path("1", "3"));
    REQUIRE(graph.is_path("1", "5"));
    REQUIRE_THROWS(graph.is_path("1", "6"));
  }
}