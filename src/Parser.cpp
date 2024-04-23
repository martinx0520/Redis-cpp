#include "Parser.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

const char DOLLAR_SIGN = '$';
const char ASTERISK_SIGN = '*';
const char PLUS_SIGN = '+';
const char MINUS_SIGN = '-';
const char COLON_SIGN = ':';

const std::string delim = "\r\n";

ParsedCommand Parser::parse_Array(const char *msg)
{
  std::string command_Str(msg), word, command;
  std::vector<std::string> parsed_Args;
  int cnt = 0;

  size_t pos_st = 0, pos_ed, delim_len = delim.length();
  while ((pos_ed = command_Str.find(delim, pos_st)) != std::string::npos)
  {
    word = command_Str.substr(pos_st, pos_ed - pos_st);
    pos_st = pos_ed + delim_len;

    std::transform(word.begin(), word.end(), word.begin(), ::tolower);
    if (cnt == 2)
    {
      command = word;
    }
    else if (cnt > 0 && cnt % 2 == 0)
    {
      parsed_Args.push_back(word);
    }
    cnt++;
  }

  return ParsedCommand{command, parsed_Args};
}

ParsedCommand Parser::parse_msg(const char *msg)
{
  char fb = msg[0];
  ParsedCommand temp;
  switch (fb)
  {
  case DOLLAR_SIGN:
    return temp;
  case ASTERISK_SIGN:
    return parse_Array(msg);
  case PLUS_SIGN:
    return temp;
  case MINUS_SIGN:
    return temp;
  case COLON_SIGN:
    return temp;
  default:
    return parse_Array(msg);
  }
}