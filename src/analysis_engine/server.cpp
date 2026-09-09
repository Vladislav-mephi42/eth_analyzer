#include "graph/graph.h"
#include "handle_strategies/strategy.h"
#include "sockets.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <nlohmann/json.hpp>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

volatile sig_atomic_t keep_running = 1;

void sigchld_handler(int signum) {
  while (waitpid(-1, NULL, WNOHANG) > 0) {
  }
}

void sigint_handler(int signum) {
  const char msg[] = "\n[INFO] Finish work ...................\n";
  write(STDOUT_FILENO, msg, sizeof(msg) - 1);
  keep_running = 0;
}

void client_handle(Socket &socket) {
  try {
    int counter = 0;
    EthGraph graph;
    std::vector<std::shared_ptr<HandleStrategy>> strategies;
    strategies.push_back(
        std::make_shared<TransitStrategy>(TransitStrategy(&graph, 5 * 60)));
    strategies.push_back(std::make_shared<CycleStrategy>(CycleStrategy(&graph)));
    strategies.push_back(std::make_shared<RatioStrategy>(RatioStrategy(&graph)));
    strategies.push_back(
        std::make_shared<FanInStrategy>(FanInStrategy(&graph, 12)));
    strategies.push_back(
        std::make_shared<FanInFanOutStrategy>(FanInFanOutStrategy(&graph, 12)));
    auto res_data = json::array();
    std::string start_address;
    while (true) {
      json data = socket.recv_json();
      if (data.contains("end")) {
        for (const auto &elem : strategies) {
          res_data.push_back(elem->report(start_address));
        }
        std::cout << "[INFO] [PID: " << getpid() << "] counter: " << counter
                  << std::endl;
        socket.send_json(res_data);
        std::cout << std::endl;
        std::cout
            << "========================RESULT OF CHECK========================"
            << std::endl;
        for (const auto &elem : res_data) {
          if (elem["res"]) {
            std::cout << "[INFO] [PID: " << getpid()
                      << "] result of check: "
                      << elem["level"].get<std::string>() << " "
                      << elem["res_string"].get<std::string>() << std::endl;
          } else {
            std::cout << "[INFO] [PID: " << getpid()
                      << "] result of check: " << elem["res_string"]
                      << std::endl;
          }
        }
        std::cout
            << "==============================================================="
            << std::endl
            << std::endl;
        break;
      } else {
        graph.add(data);
        counter += 1;
        if (counter == 1) {
          start_address = data["target_node"]["address"];
        }
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "[ERROR] [PID: " << getpid() << "] " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "[ERROR] [PID: " << getpid() << "] unknown error"
              << std::endl;
  }
}

int main() {
  try {
    int port = 7008;
    int backlog = 8;

    struct sigaction action_chld;
    memset(&action_chld, 0, sizeof(struct sigaction));
    action_chld.sa_flags = SA_RESTART;
    action_chld.sa_handler = sigchld_handler;
    sigaction(SIGCHLD, &action_chld, NULL);

    struct sigaction action_sigint;
    memset(&action_sigint, 0, sizeof(struct sigaction));
    action_sigint.sa_flags = 0;
    action_sigint.sa_handler = sigint_handler;
    sigaction(SIGINT, &action_sigint, NULL);

    Server_socket server(AF_INET, SOCK_STREAM, 0);

    server.bind(port);
    std::cout << "[INFO] [PID: " << getpid() << "] bind to port " << port
              << std::endl;
    server.listen(backlog);
    std::cout << "[INFO] [PID: " << getpid()
              << "] start listening with backlog " << backlog << std::endl;

    std::vector<pid_t> children;

    while (keep_running) {
      auto socket = server.accept();
      if (!keep_running) {
        socket.close();
        break;
      }
      std::cout << "[INFO] [PID: " << getpid() << "] accept client"
                << std::endl;
      pid_t pid = fork();
      if (pid == 0) {
        server.close();
        client_handle(socket);
        socket.close();
        _exit(0);
      } else if (pid > 0) {
        children.push_back(pid);
        socket.close();
      } else {
        std::cerr << "[ERROR] fork failed" << std::endl;
        socket.close();
      }
    }

    for (pid_t child_pid : children) {
      kill(child_pid, SIGTERM);
    }

    struct sigaction sa_default;
    memset(&sa_default, 0, sizeof(struct sigaction));
    sa_default.sa_handler = SIG_DFL;
    sigaction(SIGCHLD, &sa_default, NULL);

    while (waitpid(-1, NULL, 0) > 0 || errno == EINTR) {
    }

  } catch (const std::exception &e) {
    std::cerr << "[ERROR] " << e.what() << std::endl;
  }

  return 0;
}