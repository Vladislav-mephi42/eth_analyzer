
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

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
  void add_edge(NodeId from, NodeId to, EdgeId edg_id);
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
  std::vector<uint32_t> gas_price_gwei_v_;
  std::vector<uint32_t> method_id_v_;

public:
  EdgeId add_edge(double value_eth, uint64_t timestamp, uint32_t gas_price_gwei,
                  uint32_t method_id);
  const std::vector<double> &get_value_eth_v() const { return value_eth_v_; }
  const std::vector<uint64_t> get_timestamp_v() const { return timestamp_v_; }
  const std::vector<uint32_t> &get_gas_price_gwei_v() const {
    return gas_price_gwei_v_;
  }
  const std::vector<uint32_t> &get_method_id_v() const { return method_id_v_; }
};

class NodeStorage {
private:
  std::vector<bool> is_contract_v_;
  std::vector<uint64_t> first_seen_timestamp_v_;
  std::vector<uint32_t> nonce_v_;

public:
  NodeId add_node(bool is_contract, uint64_t first_seen_timestamp,
                  uint32_t nonce);
  const std::vector<bool> &get_is_contract_v_() const { return is_contract_v_; }
  const std::vector<uint64_t> &get_first_seen_timestamp_v_() const {
    return first_seen_timestamp_v_;
  }
  const std::vector<uint32_t> &get_nonce_v_() const { return nonce_v_; }
};
enum Colour { white, grey, black };

class EthGraph {
private:
  std::vector<int> time_v_;
  EdgeManager edg_m_;
  NodeManager node_m_;
  EdgeStorage edg_s_;
  NodeStorage node_s_;

public:
  void add_node(const std::string &wallet_address, bool is_contract,
                uint64_t first_seen_timestamp, uint32_t nonce) {
    node_m_.add_wallet(wallet_address);
    node_s_.add_node(is_contract, first_seen_timestamp, nonce);
  }
  void add_edge(const std::string &from_address, const std::string &to_address,
                double value_eth, uint64_t timestamp, uint32_t gas_price_gwei,
                uint32_t method_id) {
    NodeId from = node_m_.get_id(from_address);
    NodeId to = node_m_.get_id(to_address);
    EdgeId edg_id =
        edg_s_.add_edge(value_eth, timestamp, gas_price_gwei, method_id);
    edg_m_.add_edge(from, to, edg_id);
  }
};