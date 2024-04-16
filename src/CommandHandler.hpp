#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include "Config.hpp"
#include "Parser.hpp"
#include <map>
#include <string>

struct Entry {
  std::string length;
  std::string value;
  long long expiry;
};

class CommandHandler {
public:
  Config config;
  bool isReplica;
  std::map<std::string, Entry> data_store;

  CommandHandler(Config config, bool isReplica = false)
      : config(std::move(config)), isReplica(isReplica){};

  std::string process_commands(ParsedCommand &pc);

  bool is_write(std::string &command);
};

#endif