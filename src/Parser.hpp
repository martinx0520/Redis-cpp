#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <string>
#include <vector>

struct Entry
{
  std::string length;
  std::string value;
  long long expiry;
};

struct CommandLineEntry
{
  std::map<std::string, Entry> mp;
  int hostPort = 6379;
  std::string masterHost = "127.0.0.1";
  int masterPort = -1;
  std::string role = "master";
  std::string master_replid = "8371b4fb1155b71f4a04d3e1bc3e18c4a990aeeb";
  int master_repl_offset = 0;
  std::vector<int> replicas;
  int master_fd = -1;
};

void parse_msg(char *msg, CommandLineEntry &cliEntry, int client_fd);

void parse_Array(char *msg, CommandLineEntry &cliEntry, int client_fd);

#endif