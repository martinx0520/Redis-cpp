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
#include <utility>
#include <vector>

const char DOLLAR_SIGN = '$';
const char ASTERISK_SIGN = '*';
const char PLUS_SIGN = '+';
const char MINUS_SIGN = '-';
const char COLON_SIGN = ':';

const std::string delim = "\r\n";

long long get_time() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch())
      .count();
}

void parse_Array(char *msg, CommandLineEntry &cliEntry, int client_fd) {
  std::string command_Str(msg), word;
  std::vector<std::string> parsed_Arr;

  size_t pos_st = 0, pos_ed, delim_len = delim.length();
  while ((pos_ed = command_Str.find(delim, pos_st)) != std::string::npos) {
    word = command_Str.substr(pos_st, pos_ed - pos_st);
    pos_st = pos_ed + delim_len;

    std::transform(word.begin(), word.end(), word.begin(), ::tolower);
    parsed_Arr.push_back(word);
  }

  int arrLen = parsed_Arr.size();
  std::string return_msg{};
  if (parsed_Arr[2] == "ping") {
    return_msg = "+PONG\r\n";
  } else if (parsed_Arr[2] == "echo") {
    return_msg = parsed_Arr[3] + delim + parsed_Arr[4] + delim;
  } else if (parsed_Arr[2] == "set") {
    cliEntry.mp[parsed_Arr[4]] = {parsed_Arr[5], parsed_Arr[6]};
    if (arrLen > 7 && parsed_Arr[8] == "px") {
      cliEntry.mp[parsed_Arr[4]].expiry =
          get_time() + std::stoll(parsed_Arr[10]);
    }
    return_msg = "+OK\r\n";
  } else if (parsed_Arr[2] == "get") {
    if (cliEntry.mp.count(parsed_Arr[4])) {
      if (cliEntry.mp[parsed_Arr[4]].expiry &&
          get_time() >= cliEntry.mp[parsed_Arr[4]].expiry) {
        cliEntry.mp.erase(parsed_Arr[4]);
        return_msg = "$-1\r\n";
      } else {
        auto e = cliEntry.mp[parsed_Arr[4]];
        return_msg = e.length + delim + e.value + delim;
      }
    } else {
      return_msg = "$-1\r\n";
    }
  } else if (parsed_Arr[2] == "info") {
    std::vector<std::pair<std::string, std::string>> fields;
    if (parsed_Arr[4] == "replication") {
      fields.push_back({"role", cliEntry.role});
      for (auto p : fields) {
        return_msg += "$" +
                      std::to_string(p.first.length() + p.second.length() + 1) +
                      delim;
        return_msg += p.first + ":" + p.second + delim;
      }
    } else {
      return_msg = "+\r\n";
    }
  } else {
    return_msg = "+\r\n";
  }

  std::cout << "Response from client"
            << ": " << return_msg << " length " << return_msg.length()
            << std::endl;
  std::cout << send(client_fd, return_msg.c_str(), return_msg.length(), 0)
            << std::endl;
}

void parse_msg(char *msg, CommandLineEntry &cliEntry, int client_fd) {
  char fb = msg[0];
  switch (fb) {
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