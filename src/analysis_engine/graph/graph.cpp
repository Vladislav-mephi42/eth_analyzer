#include "graph.h"
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

NodeId NodeManager::add_wallet(const std::string &wallet_address) {
  eth_addresses_.push_back(wallet_address);
  return static_cast<uint32_t>(eth_addresses_.size() - 1);
}
NodeId NodeManager::get_id(const std::string &wallet_address) {
  auto it =
      std::find(eth_addresses_.begin(), eth_addresses_.end(), wallet_address);
  if (it == eth_addresses_.end()) {
    throw std::runtime_error(
        "Node manager doesn't contain the given wallet address");
  }
  return std::distance(eth_addresses_.begin(), it);
}
std::string NodeManager::get_address(NodeId id) {
  if (id >= eth_addresses_.size()) {
    throw std::runtime_error("invalid node id");
  }
  return eth_addresses_[id];
}
bool NodeManager::is_valid_id(NodeId id) {
  if (id >= eth_addresses_.size()) {
    return false;
  }
  return true;
}

void EdgeManager::add_edge(NodeId from, NodeId to, EdgeId edg_id) {
  if (from >= edg_from_.size()) {
    edg_from_.resize(from + 1);
  }
  if (to >= edg_to_.size()) {
    edg_to_.resize(to + 1);
  }
  edg_from_[from].push_back(std::pair(to, edg_id));
  edg_to_[to].push_back(std::pair(from, edg_id));
}
void EdgeManager::remove_edge(NodeId from, NodeId to) {
  if (from >= edg_from_.size()) {
    throw std::runtime_error("invalid id for the source vertex of the edge");
  }
  if (to >= edg_to_.size()) {
    throw std::runtime_error("invalid id for the dist vertexof the edge");
  }
  auto it = std::find_if(
      edg_from_[from].begin(), edg_from_[from].end(),
      [&to](const std::pair<NodeId, EdgeId> &x) { return x.first == to; });
  if (it == edg_from_[from].end()) {
    throw std::runtime_error("This edge doesn't exist");
  }
  edg_from_[from].erase(it);

  it = std::find_if(
      edg_to_[to].begin(), edg_to_[to].end(),
      [&from](const std::pair<NodeId, EdgeId> &x) { return x.first == from; });
  if (it == edg_to_[to].end()) {
    throw std::runtime_error(
        "Class is in an inconsistent state: class invariant violated");
  }
  edg_from_[to].erase(it);
}
std::vector<NodeId> EdgeManager::get_nodes_to(NodeId node_id) {
  if (node_id >= edg_from_.size()) {
    return std::vector<NodeId>{};
  }
  std::vector<NodeId> res;
  for (const auto elem : edg_from_[node_id]) {
    res.push_back(elem.first);
  }
  return res;
}
std::vector<NodeId> EdgeManager::get_nodes_from(NodeId node_id) {
  if (node_id >= edg_to_.size()) {
    return std::vector<NodeId>{};
  }
  std::vector<NodeId> res;
  for (const auto elem : edg_to_[node_id]) {
    res.push_back(elem.first);
  }
  return res;
}
std::vector<NodeId> EdgeManager::get_edges_from(NodeId node_id) {
  if (node_id >= edg_from_.size()) {
    return std::vector<NodeId>{};
  }
  std::vector<NodeId> res;
  for (const auto elem : edg_from_[node_id]) {
    res.push_back(elem.second);
  }
  return res;
}
std::vector<NodeId> EdgeManager::get_edges_to(NodeId node_id) {
  if (node_id >= edg_to_.size()) {
    return std::vector<NodeId>{};
  }
  std::vector<NodeId> res;
  for (const auto elem : edg_to_[node_id]) {
    res.push_back(elem.second);
  }
  return res;
}

EdgeId EdgeStorage::add_edge(double value_eth, uint64_t timestamp,
                             uint32_t gas_price_gwei, uint32_t method_id) {
  value_eth_v_.push_back(value_eth);
  timestamp_v_.push_back(timestamp);
  gas_price_gwei_v_.push_back(gas_price_gwei);
  method_id_v_.push_back(method_id);
  return value_eth_v_.size() - 1;
}
NodeId NodeStorage::add_node(bool is_contract, uint64_t first_seen_timestamp,
                             uint32_t nonce) {
  is_contract_v_.push_back(is_contract);
  first_seen_timestamp_v_.push_back(first_seen_timestamp);
  nonce_v_.push_back(nonce);
}