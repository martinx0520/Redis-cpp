#ifndef SERVER_H
#define SERVER_H

#include "CommandHandler.hpp"
#include "Config.hpp"
#include <arpa/inet.h>
#include <mutex>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>

const std::string delim = "\r\n";
struct Replica {
  int fd;
  int port;
  std::string host;
  std::chrono::steady_clock::time_point
      last_sync_time; // Last successful sync time with this replica

  // // Default constructor
  Replica()
      : fd(-1), port(0), last_sync_time(std::chrono::steady_clock::now()) {}

  Replica(int fd, std::string host, int port)
      : fd(fd), host(std::move(host)), port(port),
        last_sync_time(std::chrono::steady_clock::now()) {}
};

class Server {
public:
  Config config;
  int host_fd;
  int connection_backlog = 5;
  bool isReplica;
  std::unordered_map<int, Replica> replicas;
  std::mutex replicasMutex;
  CommandHandler RespHandler;
  std::mutex commandHandlerMutex;

  Server(Config config, CommandHandler RespHandler, bool isReplica = false)
      : config(std::move(config)), RespHandler(std::move(RespHandler)),
        isReplica(isReplica){};

  bool start();
  void request_handler(int client_fd);
  bool connect_master();
  bool initiate_handshake(int master_fd);
  void propagate_commands(const std::string &command);
  void command_loop();
  void add_replica(int fd, const std::string &host, int port);
  void remove_replica(int fd);
};

#endif