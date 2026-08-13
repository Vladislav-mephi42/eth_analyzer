#ifndef STRATEGY_H
#define STRATEGY_H

#include "../graph/graph.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stack>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

class HandleStrategy {
public:
  virtual json report(const std::string &start_address) const = 0;
};

class CycleStrategy : public HandleStrategy {
private:
  EthGraph *graph;

public:
  CycleStrategy(EthGraph *graph) : graph(graph) {}
  json report(const std::string &start_address) const override {
    json data;
    if (graph->is_cycled(start_address)) {
      data["res"] = true;
      data["level"] = "[LOW]";
      data["res_string"] =
          "Cycle was founded. Start address == " + std::string(start_address);
      return data;
    }
    data["res"] = false;
    data["res_string"] =
        "Cycle wasn't founded. Start address == " + std::string(start_address);
    return data;
  }
};

class TransitStrategy : public HandleStrategy {
private:
  EthGraph *graph;
  uint64_t min_diff;

public:
  TransitStrategy(EthGraph *graph, uint64_t min_diff)
      : graph(graph), min_diff(min_diff) {}
  json report(const std::string &start_address) const override {
    auto node_id = graph->get_node_id(start_address);
    auto edges_from_id = graph->get_edges_from(node_id);
    auto edges_to_id = graph->get_edges_to(node_id);
    const std::vector<uint64_t> &timestamp_vec_ref =
        graph->get_edges_timestamp();
    std::vector<uint64_t> to_timestamps(edges_to_id.size());
    std::vector<uint64_t> from_timestamps(edges_from_id.size());
    uint64_t to_sum = 0;
    uint64_t from_sum = 0;
    uint64_t diff = 0;
    if (edges_from_id.size() != 0 && edges_to_id.size() != 0) {
      for (int i = 0; i < std::min(edges_to_id.size(), edges_from_id.size());
           i++) {
        to_sum += timestamp_vec_ref[edges_to_id[i]];
        from_sum += timestamp_vec_ref[edges_from_id[i]];
      }
      diff = (std::max(to_sum, from_sum) - std::min(to_sum, from_sum)) /
             std::min(edges_to_id.size(), edges_from_id.size());
    }

    json data;
    if (diff < min_diff && diff != 0) {
      data["res"] = true;
      data["level"] = "[LOW]";
      data["res_string"] = "This crypto wallet may be a bot, a contract, or "
                           "another type of transit address.  Address == " +
                           std::string(start_address);
      return data;
    }
    data["res"] = false;
    data["res_string"] =
        "This crypto wallet is not necessarily a bot, a contract, or another "
        "type of transit address..  Address == " +
        std::string(start_address);
    return data;
  }
};

#endif