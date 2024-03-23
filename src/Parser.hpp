#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <string>

struct Entry {
  std::string length;
  std::string value;
  long long expiry;
};

struct CommandLineEntry {
  std::map<std::string, Entry> mp;
  int hostPort = 6379;
  int masterPort = -1;
  std::string role = "master";
};

void parse_msg(char *msg, CommandLineEntry &cliEntry, int client_fd);

void parse_Array(char *msg, CommandLineEntry &cliEntry, int client_fd);

#endif