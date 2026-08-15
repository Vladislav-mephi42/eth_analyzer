#ifndef STRATEGY_H
#define STRATEGY_H

#include "../graph/graph.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <nlohmann/json.hpp>
#include <queue>
#include <stack>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
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
class FanInStrategy : public HandleStrategy {
private:
  EthGraph *graph;
  size_t deep;
  std::unordered_set<NodeId> BFS(NodeId node_id, size_t deep) const {
    std::queue<std::pair<NodeId, size_t>> queue;
    std::unordered_set<NodeId> visited;
    queue.push({node_id, 0});
    visited.insert(node_id);
    size_t level;
    int counter = deep;
    while (true) {
      auto [node_id, level] = queue.front();
      auto it = visited.find(node_id);
      if (it != visited.end()) {
        throw std::runtime_error("cycle was founded");
      }

      queue.pop();

      std::vector<NodeId> nodes = graph->get_nodes_from(node_id);
      if (nodes.empty() || counter <= 0 || queue.empty()) {
        break;
      }
      if (level <= deep) {
        for (const auto &elem : nodes) {
          queue.push({elem, level + 1});
          visited.insert(elem);
        }
      }
    }
    return visited;
  }

public:
  FanInStrategy(EthGraph *graph, size_t deep) : graph(graph) {}
  json report(const std::string &start_address) const override {

    NodeId node_id = graph->get_node_id(start_address);

    std::vector<NodeId> nodes = graph->get_nodes_from(node_id);
    std::vector<std::unordered_set<NodeId>> sets;
    for (const auto &elem : nodes) {
      sets.push_back(BFS(elem, deep));
    }
    std::unordered_map<NodeId, size_t> map;
    std::unordered_map<NodeId, size_t> fan_in;
    for (const auto &set : sets) {
      for (const auto &elem : set) {
        map[elem] = 0;
      }
    }
    for (const auto &set : sets) {
      for (const auto &elem : set) {
        if (++map[elem] > 1) {
          fan_in[elem] = map[elem];
        }
      }
    }
    json data;
    std::string res;
    data["level"] = "[MEDIUM]";
    if (!fan_in.empty()) {
      data["res"] = true;
      for (const auto &elem : fan_in) {
        res += "Addresses/fan-in frequency:\n";
        res += graph->get_address_from_id(elem.first);
        res += " / ";
        res += std::to_string(elem.second);
      }
      data["res_string"] = res;
      return data;
    }
    data["res"] = false;
    res = "No fan-in patterns at deep: ";
    res += std::to_string(deep);
    data["res_string"] = res;
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
                           std::string(start_address) +
                           "The average difference between the receipt of "
                           "funds and their disbursement == " +
                           std::to_string(static_cast<int>(diff));
      return data;
    }
    data["res"] = false;
    data["res_string"] =
        "This crypto wallet is not necessarily a bot, a contract, or another "
        "type of transit address..  Address == " +
        std::string(start_address) +
        "The average difference between the receipt of "
        "funds and their disbursement == " +
        std::to_string(static_cast<int>(diff));
    return data;
  }
};

class RatioStrategy : public HandleStrategy {
private:
  EthGraph *graph;

public:
  RatioStrategy(EthGraph *graph) : graph(graph) {}
  json report(const std::string &start_address) const override {
    auto node_id = graph->get_node_id(start_address);
    auto edges_from_id = graph->get_edges_from(node_id);
    auto edges_to_id = graph->get_edges_to(node_id);
    auto edg_value_vec = graph->get_edges_value();
    double to_sum = 0;
    double from_sum = 0;
    double ratio = 0;
    if (edges_from_id.size() != 0 && edges_to_id.size() != 0) {
      for (int i = 0; i < std::min(edges_to_id.size(), edges_from_id.size());
           i++) {
        to_sum += edg_value_vec[edges_to_id[i]];
        from_sum += edg_value_vec[edges_from_id[i]];
      }
    }
    if (to_sum != 0 && from_sum != 0) {
      ratio = from_sum / to_sum;
    }

    json data;

    data["res"] = true;
    data["level"] = "[LOW]";
    if (ratio >= 0.90 && ratio <= 1.05) {
      data["res_string"] =
          "Address " + start_address +
          " sends and receives almost the same amount (Net Flow Ratio: " +
          std::to_string(ratio) +
          "). This is probably a bot, router contract, or transit wallet.";
    }

    else if (ratio < 0.20) {
      data["res_string"] =
          "Address " + start_address +
          " keeps most of the money it receives (Net Flow Ratio: " +
          std::to_string(ratio) +
          "). It does not spend much. This is likely a cold wallet, vault, or "
          "final collector.";
    }

    else if (ratio > 1.50) {
      data["res_string"] =
          "Address " + start_address +
          " sends out much more than it receives (Net Flow Ratio: " +
          std::to_string(ratio) +
          "). This is likely a CEX withdrawal wallet, treasury, or faucet.";
    }

    else if (ratio >= 0.20 && ratio < 0.90) {
      data["res_string"] =
          "Address " + start_address +
          " looks like a normal user (Net Flow Ratio: " +
          std::to_string(ratio) +
          "). It spends some money but keeps some on the balance.";
    }

    else if (ratio > 1.05 && ratio <= 1.50) {
      data["res_string"] =
          "Address " + start_address +
          " sends out more money than usual (Net Flow Ratio: " +
          std::to_string(ratio) +
          "). It is probably selling or moving funds to other wallets.";
    }

    else {
      data["res_string"] = "Address " + start_address +
                           " has an unusual volume ratio (Net Flow Ratio: " +
                           std::to_string(ratio) + "). It needs manual review.";
    }

    return data;
  }
};

#endif