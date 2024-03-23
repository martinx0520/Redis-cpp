#include "Parser.hpp"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <csignal>
#include <iostream>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

void request_handler(CommandLineEntry &cliEntry, int client_fd) {
  char buffer[1024] = {0};

  while (true) {
    int recvStatus = recv(client_fd, buffer, sizeof(buffer), 0);
    if (recvStatus < 0) {
      std::cerr << "Failed to receive from socket\n";
      break;
    } else if (recvStatus == 0) {
      break;
    }

    std::cout << "Message from client: " << buffer << std::endl;

    parse_msg(buffer, cliEntry, client_fd);
  }

  close(client_fd);
}

int main(int argc, char *argv[]) {
  signal(SIGPIPE, SIG_IGN);
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }

  // // Since the tester restarts your program quite often, setting REUSE_PORT
  // // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) <
      0) {
    std::cerr << "setsockopt(SO_REUSEPORT) failed\n";
    return 1;
  }

  int enable = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) <
      0) {
    std::cerr << "setsockopt(SO_REUSEADDR) failed\n";
    return 1;
  }

  struct CommandLineEntry cliEntry;
  for (int i = 0; i < argc; ++i) {
    if (argc >= 3 && i == 1 && std::string(argv[i]) == "--port") {
      cliEntry.hostPort = std::stoi(argv[i + 1]);
    }
    if (argc >= 6 && i == 3 && std::string(argv[i]) == "--replicaof") {
      cliEntry.masterPort = std::stoi(argv[i + 2]);
      cliEntry.role = "slave";
    }
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(cliEntry.hostPort);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
      0) {
    std::cerr << "Failed to bind to port " << cliEntry.hostPort << "\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n" << std::endl;

  while (true) {
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                           (socklen_t *)&client_addr_len);
    if (client_fd < 0) {
      std::cerr << "Failed to establish a connection\n";
      break;
    }
    std::cout << "Client connection established" << std::endl;

    std::thread t(request_handler, std::ref(cliEntry), client_fd);
    t.detach();
  }

  close(server_fd);

  return 0;
}
