#include "Config.hpp"

void parse_Args(int argc, char *argv[], Config &config)
{
  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];
    if (i + 1 < argc && arg == "--port")
    {
      config.hostPort = std::stoi(argv[i + 1]);
    }
    if (i + 2 < argc && arg == "--replicaof")
    {
      config.masterHost = argv[++i];
      config.masterPort = std::stoi(argv[++i]);
      config.role = "slave";
    }
  }
}