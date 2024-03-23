#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <string>

struct Entry {
  std::string length;
  std::string value;
  long long expiry;
};

void parse_msg(char *msg, std::map<std::string, Entry> &mp, int client_fd);

void parse_Array(char *msg, std::map<std::string, Entry> &mp, int client_fd);

#endif