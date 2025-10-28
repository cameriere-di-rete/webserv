#pragma once

class Server {
private:
  Server();
  Server &operator=(Server other);

public:
  Server(int port);
  Server(const Server &other);
  ~Server();

  int fd;
  int port;
};
