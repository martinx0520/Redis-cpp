#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

const char DOLLAR_SIGN = '$';
const char ASTERISK_SIGN = '*';
const char PLUS_SIGN = '+';
const char MINUS_SIGN = '-';
const char COLON_SIGN = ':';

const std::string delim = "\r\n";

std::map<std::string, std::pair<std::string, std::string>> mp;

void parse_Array(char *msg, int client_fd) {
  std::string command_Str(msg), word;
  std::vector<std::string> parsed_Arr;

  size_t pos_st = 0, pos_ed, delim_len = delim.length();
  while ((pos_ed = command_Str.find(delim, pos_st)) != std::string::npos) {
    word = command_Str.substr(pos_st, pos_ed - pos_st);
    pos_st = pos_ed + delim_len;

    std::transform(word.begin(), word.end(), word.begin(), ::tolower);
    parsed_Arr.push_back(word);
  }
  parsed_Arr.push_back(command_Str.substr(pos_st));

  char *return_msg{};
  if (parsed_Arr[2] == "ping") {
    return_msg = (char *)"+PONG\r\n";
  } else if (parsed_Arr[2] == "echo") {
    std::string response = parsed_Arr[3] + delim + parsed_Arr[4] + delim;
    return_msg = (char *)response.c_str();
  } else {
    return_msg = (char *)"+\r\n";
  }

  std::cout << "Response from client: " << return_msg << std::endl;

  send(client_fd, return_msg, strlen(return_msg), 0);
}

void parse_msg(char *msg, int client_fd) {
  char fb = msg[0];
  switch (fb) {
  case DOLLAR_SIGN:
    return;
  case ASTERISK_SIGN:
    parse_Array(msg, client_fd);
  case PLUS_SIGN:
    return;
  case MINUS_SIGN:
    return;
  case COLON_SIGN:
    return;
  default:
    std::cerr << "Not a Redis Command\n";
    return;
  }
}

void request_handler(int client_fd) {
  char buffer[1024] = {0};

  while (true) {
    int recvStatus = recv(client_fd, buffer, sizeof(buffer), 0);
    if (recvStatus < 0) {
      std::cerr << "Failed to receive from socket\n";
      break;
    }

    std::cout << "Message from client: " << buffer << std::endl;

    parse_msg(buffer, client_fd);
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

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  std::vector<std::thread> threads;

  std::cout << "Waiting for a client to connect...\n" << std::endl;

  while (true) {
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                           (socklen_t *)&client_addr_len);
    if (client_fd < 0) {
      std::cerr << "Failed to establish a connection\n";
      break;
    }
    std::cout << "Client connection established" << std::endl;

    threads.emplace_back(request_handler, client_fd);
  }

  for (auto &t : threads) {
    t.join();
  }

  close(server_fd);

  return 0;
}
