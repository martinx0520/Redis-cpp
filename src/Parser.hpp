#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

struct ParsedCommand {
  std::string command;
  std::vector<std::string> command_args;
};

class Parser {
public:
  ParsedCommand parse_msg(const char *msg);

  ParsedCommand parse_Array(const char *msg);
};

#endif