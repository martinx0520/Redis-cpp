#include "Parser.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <netdb.h>
#include <numeric>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>

const char DOLLAR_SIGN = '$';
const char ASTERISK_SIGN = '*';
const char PLUS_SIGN = '+';
const char MINUS_SIGN = '-';
const char COLON_SIGN = ':';

const std::string delim = "\r\n";
const std::string empty_rdb = "\x52\x45\x44\x49\x53\x30\x30\x31\x31\xfa\x09\x72\x65\x64\x69\x73\x2d\x76\x65\x72\x05\x37\x2e\x32\x2e\x30\xfa\x0a\x72\x65\x64\x69\x73\x2d\x62\x69\x74\x73\xc0\x40\xfa\x05\x63\x74\x69\x6d\x65\xc2\x6d\x08\xbc\x65\xfa\x08\x75\x73\x65\x64\x2d\x6d\x65\x6d\xc2\xb0\xc4\x10\x00\xfa\x08\x61\x6f\x66\x2d\x62\x61\x73\x65\xc0\x00\xff\xf0\x6e\x3b\xfe\xc0\xff\x5a\xa2";

long long get_time()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch())
      .count();
}

void parse_Array(char *msg, CommandLineEntry &cliEntry, int client_fd)
{
  std::string command_Str(msg), word;
  std::vector<std::string> parsed_Arr;

  size_t pos_st = 0, pos_ed, delim_len = delim.length();
  while ((pos_ed = command_Str.find(delim, pos_st)) != std::string::npos)
  {
    word = command_Str.substr(pos_st, pos_ed - pos_st);
    pos_st = pos_ed + delim_len;

    std::transform(word.begin(), word.end(), word.begin(), ::tolower);
    parsed_Arr.push_back(word);
  }

  int arrLen = parsed_Arr.size();
  std::string return_msg{};
  if (parsed_Arr[2] == "ping")
  {
    return_msg = "+PONG\r\n";
  }
  else if (parsed_Arr[2] == "echo")
  {
    return_msg = parsed_Arr[3] + delim + parsed_Arr[4] + delim;
  }
  else if (parsed_Arr[2] == "set")
  {
    cliEntry.mp[parsed_Arr[4]] = {parsed_Arr[5], parsed_Arr[6]};
    if (arrLen > 7 && parsed_Arr[8] == "px")
    {
      cliEntry.mp[parsed_Arr[4]].expiry =
          get_time() + std::stoll(parsed_Arr[10]);
    }
    for (auto r : cliEntry.replicas)
    {
      send(r, command_Str.c_str(), command_Str.length(), 0);
    }
    return_msg = "+OK\r\n";
  }
  else if (parsed_Arr[2] == "get")
  {
    if (cliEntry.mp.count(parsed_Arr[4]))
    {
      if (cliEntry.mp[parsed_Arr[4]].expiry &&
          get_time() >= cliEntry.mp[parsed_Arr[4]].expiry)
      {
        cliEntry.mp.erase(parsed_Arr[4]);
        return_msg = "$-1\r\n";
      }
      else
      {
        auto e = cliEntry.mp[parsed_Arr[4]];
        return_msg = e.length + delim + e.value + delim;
      }
    }
    else
    {
      return_msg = "$-1\r\n";
    }
  }
  else if (parsed_Arr[2] == "info")
  {
    std::vector<std::pair<std::string, std::string>> fields;
    if (parsed_Arr[4] == "replication")
    {
      fields.push_back({"role", cliEntry.role});
      fields.push_back({"master_replid", cliEntry.master_replid});
      fields.push_back(
          {"master_repl_offset", std::to_string(cliEntry.master_repl_offset)});
      int len = 0;
      for (auto p : fields)
      {
        len += p.first.length() + p.second.length() + 3;
        return_msg += p.first + ":" + p.second + delim;
      }
      return_msg = "$" + std::to_string(len - 2) + delim + return_msg;
    }
    else
    {
      return_msg = "+\r\n";
    }
  }
  else if (parsed_Arr[2] == "replconf")
  {
    if (parsed_Arr[4] == "listening-port")
    {
      cliEntry.replicas.push_back(client_fd);
    }
    return_msg = "+OK\r\n";
  }
  else if (parsed_Arr[2] == "psync")
  {
    return_msg = "+FULLRESYNC " + cliEntry.master_replid + " " + std::to_string(cliEntry.master_repl_offset) + delim;
    if (parsed_Arr[4] == "?" && parsed_Arr[6] == "-1")
    {
      std::string rdb_response = "$" + std::to_string(empty_rdb.length()) + delim + empty_rdb;
      return_msg += rdb_response;
    }
  }
  else
  {
    return_msg = "+\r\n";
  }

  std::cout << "Response from client"
            << ": " << return_msg << "length " << return_msg.length()
            << std::endl;
  std::cout << send(client_fd, return_msg.c_str(), return_msg.length(), 0)
            << std::endl;
  /* if (parsed_Arr[2] == "psync")
  {
    std::string rdb_in_bin = hex_to_bin(empty_rdb);
    std::string rdb_response = "$" + std::to_string(rdb_in_bin.length()) + delim + rdb_in_bin;
    send(client_fd, rdb_response.c_str(), rdb_response.size(), 0);
  } */
}

void parse_msg(char *msg, CommandLineEntry &cliEntry, int client_fd)
{
  char fb = msg[0];
  switch (fb)
  {
  case DOLLAR_SIGN:
    return;
  case ASTERISK_SIGN:
    parse_Array(msg, cliEntry, client_fd);
  case PLUS_SIGN:
    return;
  case MINUS_SIGN:
    return;
  case COLON_SIGN:
    return;
  default:
    std::cerr << "Not a Redis Command\n";
    return;
  }
}