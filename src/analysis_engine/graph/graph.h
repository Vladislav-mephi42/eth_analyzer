#ifndef GRAPH_H
#define GRAPH_H

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
  std::vector<EdgeId> get_edges_from(NodeId node_id);
  std::vector<EdgeId> get_edges_to(NodeId node_id);
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
  int size() { return timestamp_v_.size(); }
  const std::vector<double> &get_value_eth_v() const { return value_eth_v_; }
  const std::vector<uint64_t> &get_timestamp_v() const { return timestamp_v_; }
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
                uint32_t nonce);
  void add_edge(const std::string &from_address, const std::string &to_address,
                double value_eth, uint64_t timestamp);
  bool is_cycled(const std::string &start_address);
  bool is_path(const std::string &from_address, const std::string &to_address);
  void add(const json &data);
  const std::vector<double> &get_edges_value() const {
    return edg_s_.get_value_eth_v();
  }
  const std::vector<uint64_t> &get_edges_timestamp() const {
    return edg_s_.get_timestamp_v();
  }
  const std::vector<uint32_t> &get_nodes_nonce() const {
    return node_s_.get_nonce_v_();
  }
  const std::vector<uint64_t> &get_nodes_balance() const {
    return node_s_.get_balance_v_();
  }
  int node_size() { return node_s_.size(); }
  int edge_size() { return edg_s_.size(); }
  NodeId get_node_id(const std::string &wallet_address) {
    return node_m_.get_id(wallet_address);
  }
  std::string get_address_from_id(NodeId id) { return node_m_.get_address(id); }
  std::vector<NodeId> get_nodes_to(NodeId node_id) {
    return edg_m_.get_nodes_to(node_id);
  }
  std::vector<NodeId> get_nodes_from(NodeId node_id) {
    return edg_m_.get_nodes_from(node_id);
  }
  std::vector<EdgeId> get_edges_from(NodeId node_id) {
    return edg_m_.get_nodes_from(node_id);
  }
  std::vector<EdgeId> get_edges_to(NodeId node_id) {
    return edg_m_.get_edges_to(node_id);
  }
};
#endif