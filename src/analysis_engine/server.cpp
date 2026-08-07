#include "graph/graph.h"
#include "sockets.h"
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

void client_handle(Socket &socket) {
  int counter = 0;
  EthGraph graph;
  std::string start_address;
  while (true) {
    json data = socket.recv_json();
    if (data.contains("end")) {
      bool res = graph.is_cycled(start_address);
      std::cout << "RESULT OF CHECK ==== " << res << std::endl;
      break;
    } else {
      graph.add(data);
      counter += 1;
      if (counter == 1) {
        start_address = data["target_node"]["address"];
      }
      std::cout << counter << std::endl;
    }
  }
}

int main() {
  struct sigaction action_chld;
  memset(&action_chld, 0, sizeof(struct sigaction));
  action_chld.sa_flags = SA_RESTART;
  action_chld.sa_handler = sigchld_handler;
  sigaction(SIGCHLD, &action_chld, NULL);

  Server_socket server(AF_INET, SOCK_STREAM, 0);
  server.bind(7009);
  server.listen(8);
  pid_t pid;
  while (true) {
    auto socket = server.accept();
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
}