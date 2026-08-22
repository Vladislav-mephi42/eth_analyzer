#ifndef STRATEGY_H
#define STRATEGY_H

#include "../graph/graph.h"
#include <algorithm>
#include <cstdint>
#include <future>
#include <iostream>
#include <nlohmann/json.hpp>
#include <queue>
#include <set>
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

std::unordered_map<NodeId, size_t> FanInBFS(EthGraph *graph, NodeId start,
                                            size_t max_depth) {
  std::queue<std::tuple<NodeId, size_t>> queue;
  std::unordered_map<NodeId, size_t> visited;
  queue.push({start, 0});
  visited.insert({start, 1});

  while (!queue.empty()) {
    auto [current, level] = queue.front();
    queue.pop();

    if (level >= max_depth)
      continue;

    for (const auto &neighbor : graph->get_nodes_from(current)) {
      auto it = visited.find(neighbor);
      if (it == visited.end()) {
        visited.insert({neighbor, 1});
        queue.push({neighbor, level + 1});
      } else {
        it->second++;
      }
    }
  }
  return visited;
}
class FanInStrategy : public HandleStrategy {
private:
  EthGraph *graph;
  size_t deep;

public:
  FanInStrategy(EthGraph *graph, size_t deep) : graph(graph), deep(deep) {}

  json report(const std::string &start_address) const override {
    NodeId start_id = graph->get_node_id(start_address);
    std::vector<NodeId> neighbors = graph->get_nodes_from(start_id);

    std::vector<std::future<std::unordered_map<NodeId, size_t>>> futures;
    for (const auto &neighbor : neighbors) {
      futures.push_back(
          std::async(std::launch::async, FanInBFS, graph, neighbor, deep));
    }

    std::vector<std::unordered_map<NodeId, size_t>> sets;
    sets.reserve(futures.size());
    for (auto &fut : futures) {
      sets.push_back(fut.get());
    }

    std::unordered_map<NodeId, size_t> count_map;
    for (const auto &set : sets) {
      for (const auto &node : set) {

        count_map[node.first] += node.second;
      }
    }

    std::unordered_map<NodeId, size_t> fan_in;
    for (const auto &elem : count_map) {
      if (elem.second > 1) {

        fan_in[elem.first] = elem.second;
      }
    }

    json data;
    data["level"] = "[LOW]";
    std::string res;

    if (fan_in.size() > 0) {
      data["res"] = true;
      res += "\nThe Fan-in pattern is a scheme where several crypto wallets "
             "from the analyzed dependency graph send Ethereum to the same "
             "wallet, and it DOES NOT MATTER at what level from the starting "
             "address this happens.\nAddresses/fan-in frequency:\n";
      for (const auto &[node, count] : fan_in) {
        res += graph->get_address_from_id(node);
        res += " / ";
        res += std::to_string(count);
        res += "\n";
      }
      data["res_string"] = res;

      return data;
    }
    data["res"] = false;
    res = "No fan-in patterns at deep: ";
    res += "\nThe Fan-in pattern is a scheme where several crypto wallets from "
           "the analyzed dependency graph send Ethereum to the same wallet, "
           "and it DOES NOT MATTER at what level from the starting address "
           "this happens.";
    res += std::to_string(deep);
    data["res_string"] = res;
    return data;
  }
};
std::unordered_set<NodeId> FanInFanOutBFS(EthGraph *graph, NodeId start,
                                          size_t max_depth) {
  std::queue<std::tuple<NodeId, size_t>> queue;
  std::unordered_set<NodeId> visited;
  queue.push({start, 0});
  visited.insert({start, 1});

  while (!queue.empty()) {
    auto [current, level] = queue.front();
    queue.pop();

    if (level >= max_depth)
      continue;

    for (const auto &neighbor : graph->get_nodes_from(current)) {
      auto it = visited.find(neighbor);
      if (it == visited.end()) {
        visited.insert(neighbor);
        queue.push({neighbor, level + 1});
      }
    }
  }
  return visited;
}
class FanInFanOutStrategy : public HandleStrategy {
private:
  EthGraph *graph;
  size_t deep;

public:
  FanInFanOutStrategy(EthGraph *graph, size_t deep)
      : graph(graph), deep(deep) {}

  json report(const std::string &start_address) const override {
    NodeId start_id = graph->get_node_id(start_address);
    std::vector<NodeId> neighbors = graph->get_nodes_from(start_id);

    std::vector<std::future<std::unordered_set<NodeId>>> futures;
    for (const auto &neighbor : neighbors) {
      futures.push_back(std::async(std::launch::async, FanInFanOutBFS, graph,
                                   neighbor, deep));
    }

    std::vector<std::unordered_set<NodeId>> sets;
    sets.reserve(futures.size());
    for (auto &fut : futures) {
      sets.push_back(fut.get());
    }

    std::unordered_map<NodeId, size_t> count_map;
    for (const auto &set : sets) {
      for (const auto &node : set) {
        count_map[node] += 1;
      }
    }

    std::unordered_map<NodeId, size_t> fan_in_fan_out;
    for (const auto &elem : count_map) {
      if (elem.second > 1) {

        fan_in_fan_out[elem.first] = elem.second;
      }
    }

    json data;
    data["level"] = "\n[MEDIUM]";
    std::string res;

    if (fan_in_fan_out.size() > 0) {
      data["res"] = true;
      res +=
          "\nThe Fan-in-Fan-out pattern is a scheme where several crypto "
          "wallets from the analyzed dependency graph send Ethereum to the "
          "same wallet, and it IS IMPORTANT that the graph is initially "
          "divided into subgraphs whose vertices are the neighbors of the "
          "starting address, and what is looked for is the number of branches "
          "that intersect.\nAddresses / fan - in - fan - out frequency :\n ";
      for (const auto &[node, count] : fan_in_fan_out) {
        res += graph->get_address_from_id(node);
        res += " / ";
        res += std::to_string(count);
        res += "\n";
      }
      data["res_string"] = res;

      return data;
    }
    data["res"] = false;
    res = "No fan-in-fan-out patterns at deep: ";
    res += std::to_string(deep);
    res +=
        "\nThe Fan-in-Fan-out pattern is a scheme where several crypto wallets "
        "from the analyzed dependency graph send Ethereum to the same wallet, "
        "and it IS IMPORTANT that the graph is initially divided into "
        "subgraphs whose vertices are the neighbors of the starting address, "
        "and what is looked for is the number of branches that intersect.";
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