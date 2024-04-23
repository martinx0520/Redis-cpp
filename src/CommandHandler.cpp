#include "CommandHandler.hpp"
#include "Parser.hpp"
#include <chrono>
#include <string>
#include <sys/socket.h>
#include <vector>
const std::string empty_rdb =
    "\x52\x45\x44\x49\x53\x30\x30\x31\x31\xfa\x09\x72\x65\x64\x69\x73\x2d\x76"
    "\x65\x72\x05\x37\x2e\x32\x2e\x30\xfa\x0a\x72\x65\x64\x69\x73\x2d\x62\x69"
    "\x74\x73\xc0\x40\xfa\x05\x63\x74\x69\x6d\x65\xc2\x6d\x08\xbc\x65\xfa\x08"
    "\x75\x73\x65\x64\x2d\x6d\x65\x6d\xc2\xb0\xc4\x10\x00\xfa\x08\x61\x6f\x66"
    "\x2d\x62\x61\x73\x65\xc0\x00\xff\xf0\x6e\x3b\xfe\xc0\xff\x5a\xa2";
const std::string delim = "\r\n";

long long get_time()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch())
      .count();
}

std::string CommandHandler::process_commands(ParsedCommand &pc, int client_fd)
{

  std::string return_msg{};
  int args_Len = pc.command_args.size();
  if (pc.command == "ping")
  {
    return_msg = "+PONG\r\n";
  }
  else if (pc.command == "echo")
  {
    return_msg = "$" + std::to_string(pc.command_args[0].length()) + delim +
                 pc.command_args[0] + delim;
  }
  else if (pc.command == "set")
  {
    this->data_store[pc.command_args[0]] = {
        std::to_string(pc.command_args[1].length()), pc.command_args[1]};
    if (args_Len > 3 && pc.command_args[2] == "px")
    {
      this->data_store[pc.command_args[0]].expiry =
          get_time() + std::stoll(pc.command_args[3]);
    }

    return_msg = "+OK\r\n";
  }
  else if (pc.command == "get")
  {
    if (this->data_store.count(pc.command_args[0]))
    {
      if (this->data_store[pc.command_args[0]].expiry &&
          get_time() >= this->data_store[pc.command_args[0]].expiry)
      {
        this->data_store.erase(pc.command_args[0]);
        return_msg = "$-1\r\n";
      }
      else
      {
        auto e = this->data_store[pc.command_args[0]];
        return_msg = "$" + e.length + delim + e.value + delim;
      }
    }
    else
    {
      return_msg = "$-1\r\n";
    }
  }
  else if (pc.command == "info")
  {
    std::vector<std::pair<std::string, std::string>> fields;
    if (pc.command_args[0] == "replication")
    {
      fields.push_back({"role", this->config.role});
      fields.push_back({"master_replid", this->config.master_replid});
      fields.push_back({"master_repl_offset",
                        std::to_string(this->config.master_repl_offset)});
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
  else if (pc.command == "replconf")
  {
    if (pc.command_args[0] == "getack")
    {
      std::vector<std::string> args = {"REPLCONF", "ACK", std::to_string(this->config.master_repl_offset)};
      return_msg = return_msg + "*" + std::to_string(args.size()) + delim;
      for (auto s : args)
      {
        return_msg = return_msg +"$" + std::to_string(s.length()) + delim + s + delim;
      }
    }
    else
    {
      return_msg = "+OK\r\n";
    }
  }
  else if (pc.command == "psync")
  {
    return_msg = "+FULLRESYNC " + this->config.master_replid + " " +
                 std::to_string(this->config.master_repl_offset) + delim;
    send(client_fd, return_msg.c_str(), return_msg.length(), 0);
    return_msg = "+\r\n";
    if (pc.command_args[0] == "?" && pc.command_args[1] == "-1")
    {
      return_msg = "$" + std::to_string(empty_rdb.length()) + delim + empty_rdb;
    }
  }
  else
  {
    return_msg = "+\r\n";
  }

  return return_msg;
}

bool CommandHandler::is_write(std::string &command)
{
  return command == "set" || command == "del" || command == "flushdb";
}
