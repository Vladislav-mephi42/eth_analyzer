#define CATCH_CONFIG_MAIN
#include "graph.h"
#include "strategy.h"
#include <catch2/catch_all.hpp>
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <thread>

using json = nlohmann::json;

TEST_CASE("Graph") {
  SECTION("default") {
    EthGraph graph;
    REQUIRE_NOTHROW(graph.add_node("1", 0, 0));
    REQUIRE_NOTHROW(graph.add_node("2", 0, 0));
    REQUIRE(graph.size() == 2);
    REQUIRE_NOTHROW(graph.add_edge("1", "2", 0, 0));
    REQUIRE(graph.size() == 2);
  }
  SECTION("is_cycled(1)") {
    EthGraph graph;
    graph.add_node("1", 0, 0);
    graph.add_node("2", 0, 0);
    graph.add_node("3", 0, 0);
    graph.add_node("4", 0, 0);
    graph.add_node("5", 0, 0);
    graph.add_edge("1", "2", 0, 0);
    REQUIRE(graph.is_cycled("1") == false);

    graph.add_edge("2", "3", 0, 0);
    graph.add_edge("3", "4", 0, 0);
    graph.add_edge("4", "5", 0, 0);
    graph.add_edge("5", "1", 0, 0);
    REQUIRE(graph.is_cycled("1") == true);
  }
  SECTION("is_path(1)") {
    EthGraph graph;
    graph.add_node("1", 0, 0);
    graph.add_node("2", 0, 0);
    graph.add_node("3", 0, 0);
    graph.add_node("4", 0, 0);
    graph.add_node("5", 0, 0);
    graph.add_edge("1", "2", 0, 0);
    REQUIRE(graph.is_path("1", "2"));
    graph.add_edge("2", "3", 0, 0);
    graph.add_edge("3", "4", 0, 0);
    graph.add_edge("4", "5", 0, 0);
    graph.add_edge("5", "1", 0, 0);
    REQUIRE(graph.is_path("1", "3"));
    REQUIRE(graph.is_path("1", "5"));
    REQUIRE_THROWS(graph.is_path("1", "6"));
  }
}

TEST_CASE("Strategies") {
  SECTION("CycleStrategy(1)") {
    EthGraph graph;
    CycleStrategy strategy(&graph);
    graph.add_node("1", 0, 0);
    graph.add_node("2", 0, 0);
    graph.add_node("3", 0, 0);
    graph.add_node("4", 0, 0);
    graph.add_node("5", 0, 0);

    graph.add_edge("1", "2", 0, 0);
    graph.add_edge("2", "3", 0, 0);
    graph.add_edge("3", "4", 0, 0);
    graph.add_edge("4", "5", 0, 0);
    graph.add_edge("5", "3", 0, 0);
    auto data = strategy.report("1");
    REQUIRE(data["res"] == true);
  }
  SECTION("CycleStrategy(2)") {
    EthGraph graph;
    CycleStrategy strategy(&graph);
    graph.add_node("1", 0, 0);
    graph.add_node("2", 0, 0);
    graph.add_node("3", 0, 0);
    graph.add_node("4", 0, 0);
    graph.add_node("5", 0, 0);

    graph.add_edge("1", "2", 0, 0);
    graph.add_edge("2", "3", 0, 0);
    graph.add_edge("3", "4", 0, 0);
    graph.add_edge("4", "5", 0, 0);
    auto data = strategy.report("1");
    REQUIRE(data["res"] == false);
  }
  SECTION("TransitStrategy(1)") {
    EthGraph graph;
    TransitStrategy strategy(&graph, 4);
    graph.add_node("1", 0, 0);
    graph.add_node("2", 0, 0);
    graph.add_node("3", 0, 0);
    graph.add_node("4", 0, 0);
    graph.add_node("5", 0, 0);
    graph.add_node("6", 0, 0);
    graph.add_node("7", 0, 0);
    graph.add_node("8", 0, 0);
    graph.add_node("9", 0, 0);
    graph.add_node("10", 0, 0);

    graph.add_edge("1", "2", 0, 6);
    graph.add_edge("1", "3", 0, 7);
    graph.add_edge("1", "4", 0, 8);
    graph.add_edge("1", "5", 0, 9);
    graph.add_edge("1", "6", 0, 10);

    graph.add_edge("7", "1", 0, 1);
    graph.add_edge("8", "1", 0, 2);
    graph.add_edge("9", "1", 0, 1);
    graph.add_edge("10", "1", 0, 3);

    auto data = strategy.report("1");
    REQUIRE(data["res"] == false);
  }

  SECTION("TransitStrategy(2)") {
    EthGraph graph;

    graph.add_node("1", 0, 0);
    graph.add_node("2", 0, 0);
    graph.add_node("3", 0, 0);
    graph.add_node("4", 0, 0);
    graph.add_node("5", 0, 0);
    graph.add_node("6", 0, 0);
    graph.add_node("7", 0, 0);
    graph.add_node("8", 0, 0);
    graph.add_node("9", 0, 0);
    graph.add_node("10", 0, 0);

    graph.add_edge("1", "2", 0, 1);
    graph.add_edge("1", "3", 0, 2);
    graph.add_edge("1", "4", 0, 3);
    graph.add_edge("1", "5", 0, 2);
    graph.add_edge("1", "6", 0, 1);

    graph.add_edge("7", "1", 0, 1);
    graph.add_edge("8", "1", 0, 2);
    graph.add_edge("9", "1", 0, 1);
    graph.add_edge("10", "1", 0, 3);
    TransitStrategy strategy(&graph, 4);
    auto data = strategy.report("1");
    REQUIRE(data["res"] == true);
  }
}