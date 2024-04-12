#ifndef SERVER_H
#define SERVER_H

#include "Config.hpp"
#include "Parser.hpp"
#include <map>
#include <string>
#include <vector>

struct Entry {
  std::string length;
  std::string value;
  long long expiry;
};

class Server {
public:
  Config config;
  ParsedCommand parsed_commands;
  std::vector<std::string> replicas;
  std::map<std::string, std::string> data_store;

  void process_commands();
};

#endif