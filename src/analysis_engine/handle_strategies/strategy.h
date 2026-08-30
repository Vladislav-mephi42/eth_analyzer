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
  json report(const std::string &start_address) const override;
};

class FanInStrategy : public HandleStrategy {
private:
  EthGraph *graph;
  size_t deep;

public:
  FanInStrategy(EthGraph *graph, size_t deep) : graph(graph), deep(deep) {}

  json report(const std::string &start_address) const override;
};

class FanInFanOutStrategy : public HandleStrategy {
private:
  EthGraph *graph;
  size_t deep;

public:
  FanInFanOutStrategy(EthGraph *graph, size_t deep)
      : graph(graph), deep(deep) {}

  json report(const std::string &start_address) const override;
};
class TransitStrategy : public HandleStrategy {
private:
  EthGraph *graph;
  uint64_t min_diff;

public:
  TransitStrategy(EthGraph *graph, uint64_t min_diff)
      : graph(graph), min_diff(min_diff) {}
  json report(const std::string &start_address) const override;
};

class RatioStrategy : public HandleStrategy {
private:
  EthGraph *graph;

public:
  RatioStrategy(EthGraph *graph) : graph(graph) {}
  json report(const std::string &start_address) const override;
};

#endif