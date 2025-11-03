#pragma once

class Server {
public:
  Server(void);
  Server(int port);
  Server(const Server &other);
  ~Server();

  Server &operator=(const Server &other);

  int fd;
  int port;

  void init(void);
  void disconnect(void);
};
