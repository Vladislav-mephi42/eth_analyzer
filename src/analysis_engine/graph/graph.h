
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
using json = nlohmann::json;

using NodeId = uint32_t;
using EdgeId = uint32_t;

class NodeManager {
private:
  std::vector<std::string> eth_addresses_;

public:
  NodeId add_wallet(const std::string &wallet_address);
  NodeId get_id(const std::string &wallet_address);
  std::string get_address(NodeId id);
  bool is_valid_id(NodeId id);
};

class EdgeManager {
private:
  std::vector<std::vector<std::pair<NodeId, EdgeId>>> edg_to_;
  std::vector<std::vector<std::pair<NodeId, EdgeId>>> edg_from_;

public:
  int add_edge(NodeId from, NodeId to, EdgeId edg_id);
  void remove_edge(NodeId from, NodeId to);
  std::vector<NodeId> get_nodes_to(NodeId node_id);
  std::vector<NodeId> get_nodes_from(NodeId node_id);
  std::vector<NodeId> get_edges_from(NodeId node_id);
  std::vector<NodeId> get_edges_to(NodeId node_id);
};

class EdgeStorage {
private:
  std::vector<double> value_eth_v_;
  std::vector<uint64_t> timestamp_v_;

public:
  EdgeId add_edge(double value_eth, uint64_t timestamp);
  void pop_edge() {
    value_eth_v_.pop_back();
    timestamp_v_.pop_back();
  }
  const std::vector<double> &get_value_eth_v() const { return value_eth_v_; }
  const std::vector<uint64_t> get_timestamp_v() const { return timestamp_v_; }
};

class NodeStorage {
private:
  std::vector<uint64_t> balance_v_;
  std::vector<uint32_t> nonce_v_;

public:
  NodeId add_node(uint64_t balance, uint32_t nonce);

  const std::vector<uint32_t> &get_nonce_v_() const { return nonce_v_; }
  const std::vector<uint64_t> &get_balance_v_() const { return balance_v_; }
  int size() { return nonce_v_.size(); }
};
enum Colour { white, grey, black };

class EthGraph {
private:
  std::vector<int> time_v_;
  EdgeManager edg_m_;
  NodeManager node_m_;
  EdgeStorage edg_s_;
  NodeStorage node_s_;
  std::vector<std::pair<NodeId, Colour>> visit_map_;

public:
  int size() { return node_s_.size(); }
  void add_node(const std::string &wallet_address, uint64_t balance,
                uint32_t nonce) {
    auto id = node_m_.add_wallet(wallet_address);
    if (id >= 0) {
      node_s_.add_node(balance, nonce);
    }
  }
  void add_edge(const std::string &from_address, const std::string &to_address,
                double value_eth, uint64_t timestamp) {
    NodeId from = node_m_.get_id(from_address);
    NodeId to = node_m_.get_id(to_address);
    EdgeId edg_id = edg_s_.add_edge(value_eth, timestamp);
    if (edg_m_.add_edge(from, to, edg_id) == -1) {
      edg_s_.pop_edge();
    }
  }
  bool is_cycled(const std::string &start_address) {
    NodeId node_id = node_m_.get_id(start_address);
    std::vector<NodeId> stack;
    stack.push_back(node_id);
    while (true) {
      node_id = stack.back();
      auto nodes = edg_m_.get_nodes_from(node_id);
      if (nodes.size() == 0) {
        break;
      }
      for (NodeId elem : nodes) {
        auto it = std::find(stack.begin(), stack.end(), elem);
        if (it != stack.end()) {
          return true;
        }
        stack.push_back(elem);
      }
    }
    return false;
  }
  bool is_path(const std::string &from_address, const std::string &to_address) {
    NodeId from_id = node_m_.get_id(from_address);
    NodeId to_id = node_m_.get_id(to_address);
    std::vector<NodeId> stack;
    stack.push_back(from_id);
    while (true) {
      auto node_id = stack.back();
      auto nodes = edg_m_.get_nodes_from(node_id);
      if (nodes.size() == 0) {
        break;
      }
      for (NodeId elem : nodes) {
        if (elem == to_id) {
          return true;
        }
        auto it = std::find(stack.begin(), stack.end(), elem);
        if (it != stack.end()) {
          throw std::runtime_error("cycle founded");
        }
        stack.push_back(elem);
      }
    }
    return false;
  }

  void add(const json &data) {

    add_node(data["target_node"]["address"],
             data["target_node"]["balance"].get<uint64_t>(),
             data["target_node"]["nonce"].get<uint32_t>());
    for (const auto &elem : data["neighbours_data"]) {

      add_node(elem["address"], elem["balance"].get<uint64_t>(),
               elem["nonce"].get<uint32_t>());
    }
    for (const auto &elem : data["edges"]) {
      add_edge(elem["from"], elem["to"], elem["value"].get<double>(),
               elem["timestamp"].get<uint64_t>());
    }
  }
};