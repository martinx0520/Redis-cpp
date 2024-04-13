#include "Config.hpp"
#include "Parser.hpp"
#include "Server.hpp"
#include <arpa/inet.h>
#include <csignal>
#include <iostream>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

int main(int argc, char *argv[])
{
  signal(SIGPIPE, SIG_IGN);

  Config serverConfig;
  parse_Args(argc, argv, serverConfig);

  Server server(serverConfig);

  if (!server.start())
  {
    return 1;
  }

  server.command_loop();

  return 0;
}
