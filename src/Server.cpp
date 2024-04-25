#include "Server.hpp"
#include "Parser.hpp"
#include <cstring>
#include <iostream>
#include <sstream>
#include <thread>
#include <regex>

std::vector<std::string> split(const std::string &s, const std::regex pattern)
{
  std::vector<std::string> tokens;
  std::string word;
  std::sregex_iterator begin(s.begin(), s.end(), pattern);
  std::sregex_iterator end;

  size_t pos_st = 0;
  for (std::sregex_iterator i = begin; i != end; ++i)
  {
    std::smatch match = *i;
    word = s.substr(pos_st, match.position() - pos_st);
    pos_st = match.position();
    tokens.push_back(word);
  }
  word = s.substr(pos_st, s.length() - pos_st);
  tokens.push_back(word);
  return tokens;
}

void Server::request_handler(int client_fd)
{
  char buffer[1024] = {0};

  while (true)
  {
    int recvBytes = recv(client_fd, buffer, sizeof(buffer), 0);
    if (recvBytes < 0)
    {
      std::cerr << "Failed to receive from socket\n";
      break;
    }
    else if (recvBytes == 0)
    {
      break;
    }
    std::regex pattern(R"(\*[0-9])");
    std::string buffer_Str = std::string(buffer, recvBytes);

    std::cout << "Message received: " << buffer_Str << std::endl;

    std::lock_guard<std::mutex> lock(this->commandHandlerMutex);

    std::vector<std::string> splitted_commands =
        split(buffer_Str, pattern);

    for (int i = 0; i < splitted_commands.size(); ++i)
    {
      if (i == 0)
        continue;

      Parser RespParser;
      ParsedCommand pc = RespParser.parse_msg(splitted_commands[i].c_str());

      if (pc.command == "psync" && !this->isReplica)
      {
        add_replica(client_fd, "localhost", this->config.hostPort);
      }

      if (!this->isReplica && this->replicas.size() > 0 &&
          this->RespHandler.is_write(pc.command))
      {
        std::cout << "Propagating command to replicas\n";
        propagate_commands(std::string(buffer));
      }

      std::string return_msg = this->RespHandler.process_commands(pc, client_fd, splitted_commands[i].length(), client_fd == this->config.master_fd);

      if (return_msg != "")
      {
        std::cout << "Response from client"
                  << ": " << return_msg << std::endl;
        send(client_fd, return_msg.c_str(), return_msg.length(), 0);
      }
    }
  }

  close(client_fd);
}

bool Server::start()
{
  int host_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (host_fd < 0)
  {
    std::cerr << "Failed to create server socket\n";
    return false;
  }
  this->host_fd = host_fd;
  // // Since the tester restarts your program quite often, setting REUSE_PORT
  // // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(host_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) <
      0)
  {
    std::cerr << "setsockopt(SO_REUSEPORT) failed\n";
    return false;
  }

  int enable = 1;
  if (setsockopt(host_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) <
      0)
  {
    std::cerr << "setsockopt(SO_REUSEADDR) failed\n";
    return false;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(this->config.hostPort);

  if (bind(host_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
      0)
  {
    std::cerr << "Failed to bind to port " << this->config.hostPort << "\n";
    return false;
  }

  if (listen(host_fd, this->connection_backlog) != 0)
  {
    std::cerr << "listen failed\n";
    return false;
  }

  if (this->isReplica)
  {
    if (!connect_master())
    {
      return false;
    }
  }

  return true;
}

void Server::command_loop()
{
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n"
            << std::endl;

  while (true)
  {
    int client_fd = accept(this->host_fd, (struct sockaddr *)&client_addr,
                           (socklen_t *)&client_addr_len);
    if (client_fd < 0)
    {
      std::cerr << "Failed to establish a connection\n";
      break;
    }
    std::cout << "Client connection established" << std::endl;

    std::thread t(&Server::request_handler, this, client_fd);
    t.detach();
  }

  close(this->host_fd);
}

bool Server::connect_master()
{
  int master_fd = socket(AF_INET, SOCK_STREAM, 0);
  this->config.master_fd = master_fd;

  if (master_fd < 0)
  {
    std::cerr << "Failed to create server socket\n";
    return false;
  }

  struct sockaddr_in master_addr;
  memset(&master_addr, 0, sizeof(master_addr));
  master_addr.sin_family = AF_INET;
  master_addr.sin_port = htons(this->config.masterPort);

  if (this->config.masterHost == "localhost")
  {
    this->config.masterHost = "127.0.0.1";
  }

  if (inet_pton(AF_INET, this->config.masterHost.c_str(),
                &master_addr.sin_addr) <= 0)
  {
    std::cerr << "Invalid address/ Address not supported\n";
    return false;
  }

  if (connect(master_fd, (struct sockaddr *)&master_addr, sizeof(master_addr)) <
      0)
  {
    std::cerr << "Failed to connect to master";
    return false;
  }

  if (!initiate_handshake(master_fd))
  {
    std::cerr << "Failed to initiate handshake process with master";
    close(master_fd);
    return false;
  }

  return true;
}

bool Server::initiate_handshake(int master_fd)
{
  char hsBuffer[1024] = {0};

  // Handshake part 1
  std::string ping{"*1\r\n$4\r\nping\r\n"};

  send(master_fd, ping.c_str(), ping.size(), 0);
  int recv_msg = recv(master_fd, hsBuffer, sizeof(hsBuffer), 0);

  // Handshake part 2
  std::vector<std::string> replConf1{"REPLCONF", "listening-port",
                                     std::to_string(this->config.hostPort)};
  std::vector<std::string> replConf2{"REPLCONF", "capa", "psync2"};
  std::string res_1 = "*3" + delim, res_2 = "*3" + delim;
  for (auto s : replConf1)
  {
    res_1 += "$" + std::to_string(s.length()) + delim + s + delim;
  }
  for (auto s : replConf2)
  {
    res_2 += "$" + std::to_string(s.length()) + delim + s + delim;
  }
  send(master_fd, res_1.c_str(), res_1.size(), 0);
  recv_msg = recv(master_fd, hsBuffer, sizeof(hsBuffer), 0);

  send(master_fd, res_2.c_str(), res_2.size(), 0);
  recv_msg = recv(master_fd, hsBuffer, sizeof(hsBuffer), 0);

  // Handshake part 3
  std::vector<std::string> psync{"PSYNC", "?", "-1"};
  std::string res_psync = "*3" + delim;
  for (auto s : psync)
  {
    res_psync += "$" + std::to_string(s.length()) + delim + s + delim;
  }
  send(master_fd, res_psync.c_str(), res_psync.size(), 0);

  std::thread t(&Server::request_handler, this, master_fd);
  t.detach();

  return true;
}

void Server::add_replica(int fd, const std::string &host, int port)
{
  std::cout << "Adding replica with fd: " << fd << " host: " << host
            << " port: " << port << std::endl;
  std::lock_guard<std::mutex> lock(this->replicasMutex);
  this->replicas[fd] = Replica(fd, host, port);
}

void Server::remove_replica(int fd)
{
  close(fd);
  this->replicas.erase(fd);
}

void Server::propagate_commands(const std::string &command)
{
  for (auto r : this->replicas)
  {
    send(r.first, command.c_str(), command.length(), 0);
  }
}