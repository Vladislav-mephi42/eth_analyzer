#include "graph/graph.h"
#include "sockets.h"
#include <cstdlib>
#include <cstring>
#include <nlohmann/json.hpp>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <thread>
void sigchld_handler(int signum) {
  while (waitpid(-1, NULL, WNOHANG) > 0) {
  }
}

void sigint_handler(int signum) {
  std::cout << "\n[INFO] Finish work ..................." << std::endl;
  exit(0);
}

void client_handle(Socket &socket) {
  int counter = 0;
  EthGraph graph;
  std::string start_address;
  while (true) {
    json data = socket.recv_json();
    if (data.contains("end")) {
      bool res = graph.is_cycled(start_address);
      std::cout << "[INFO] counter: " << counter << std::endl;
      std::cout << "[INFO] result of check: " << res << std::endl;
      break;
    } else {
      graph.add(data);
      counter += 1;
      if (counter == 1) {
        start_address = data["target_node"]["address"];
      }
    }
  }
}

int main() {
  try {
    int port = 7009;
    int backlog = 8;

    struct sigaction action_chld;
    memset(&action_chld, 0, sizeof(struct sigaction));
    action_chld.sa_flags = SA_RESTART;
    action_chld.sa_handler = sigchld_handler;
    sigaction(SIGCHLD, &action_chld, NULL);

    struct sigaction action_sigint;
    memset(&action_sigint, 0, sizeof(struct sigaction));
    action_sigint.sa_flags = SA_RESTART;
    action_sigint.sa_handler = sigint_handler;
    sigaction(SIGINT, &action_sigint, NULL);

    Server_socket server(AF_INET, SOCK_STREAM, 0);

    server.bind(port);
    std::cout << "[INFO] bind to port " << port << std::endl;
    server.listen(backlog);
    std::cout << "[INFO] start listening with backlog " << backlog << std::endl;
    pid_t pid;
    while (true) {
      auto socket = server.accept();
      std::cout << "[INFO] accept client" << std::endl;
      pid = fork();
      if (pid == 0) {
        server.close();
        client_handle(socket);
        socket.close();
        break;
      }
      if (pid == 0) {
        break;
      }
      socket.close();
    }
  } catch (const std::exception &e) {
    std::cout << "[ERROR] " << e.what() << std::endl;
  }
}