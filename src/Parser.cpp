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

void parse_Array(char *msg, std::map<std::string, Entry> &mp, int client_fd) {
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
  char *return_msg{};
  if (parsed_Arr[2] == "ping") {
    return_msg = (char *)"+PONG\r\n";
  } else if (parsed_Arr[2] == "echo") {
    std::string response = parsed_Arr[3] + delim + parsed_Arr[4] + delim;
    return_msg = (char *)response.c_str();
  } else if (parsed_Arr[2] == "set") {
    mp[parsed_Arr[4]] = {parsed_Arr[5], parsed_Arr[6]};
    if (arrLen > 7 && parsed_Arr[8] == "px") {
      mp[parsed_Arr[4]].expiry = get_time() + std::stoll(parsed_Arr[10]);
    }
    return_msg = (char *)"+OK\r\n";
  } else if (parsed_Arr[2] == "get") {
    if (mp.count(parsed_Arr[4])) {
      if (mp[parsed_Arr[4]].expiry && get_time() >= mp[parsed_Arr[4]].expiry) {
        mp.erase(parsed_Arr[4]);
        return_msg = (char *)"$-1\r\n";
      } else {
        auto e = mp[parsed_Arr[4]];
        std::string get_response = e.length + delim + e.value + delim;
        return_msg = (char *)get_response.c_str();
      }
    } else {
      return_msg = (char *)"$-1\r\n";
    }
  } else if (parsed_Arr[2] == "info") {
    std::vector<std::string> responses;
    if (parsed_Arr[4] == "replication") {
      responses = {"$11", "# replication", "role:master"};
      std::string info_response = std::accumulate(
          std::begin(responses), std::end(responses), std::string(),
          [](std::string ss, std::string s) { return ss + delim + s; });
      return_msg = (char *)info_response.c_str();
      std::cout << "joined msg: " << return_msg << std::endl;
    } else {
      return_msg = (char *)"+\r\n";
    }
  } else {
    return_msg = (char *)"+\r\n";
  }

  std::cout << "Response from client"
            << ": " << return_msg << " length " << strlen(return_msg)
            << std::endl;
  std::cout << send(client_fd, return_msg, strlen(return_msg), 0) << std::endl;
}

void parse_msg(char *msg, std::map<std::string, Entry> &mp, int client_fd) {
  char fb = msg[0];
  switch (fb) {
  case DOLLAR_SIGN:
    return;
  case ASTERISK_SIGN:
    parse_Array(msg, mp, client_fd);
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