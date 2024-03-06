#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <pthread.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

void request_handler(int client_fd) {
  char buffer[1024] = {0};
  const char *resp = "+PONG\r\n";

  while (true) {
    int recvStatus = recv(client_fd, buffer, sizeof(buffer), 0);
    if (recvStatus < 0) {
      std::cerr << "Failed to receive from socket\n";
      break;
    }
    std::cout << "Message from client: " << buffer << std::endl;

    send(client_fd, resp, strlen(resp), 0);
  }

  close(client_fd);
}

int main(int argc, char **argv) {

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
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
      0) {
    std::cerr << "Failed to bind to port 6379\n";
    return 1;
  }

  int connection_backlog = 50;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n" << std::endl;

  int client_fd{};
  std::thread t;

  while (true) {
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                       (socklen_t *)&client_addr_len);
    if (client_fd < 0) {
      std::cerr << "Failed to establish a connection\n";
      continue;
    }
    std::cout << "Client connection established" << std::endl;

    t = std::thread(request_handler, client_fd);
    t.detach();
  }

  close(server_fd);

  return 0;
}
