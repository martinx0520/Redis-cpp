#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>

struct Config {
  int hostPort = 6379;
  int masterPort = -1;
  int master_repl_offset = 0;
  int master_fd = -1;
  std::string masterHost = "127.0.0.1";
  std::string role = "master";
  std::string master_replid = "8371b4fb1155b71f4a04d3e1bc3e18c4a990aeeb";
};

void parse_Args(int argc, char *argv[], Config &config);

#endif