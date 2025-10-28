#pragma once

class Server {
private:
public:
  Server(void);
  Server(int port);
  Server(const Server &other);
  ~Server();

  Server &operator=(Server other);

  int fd;
  int port;

  void init(void);
  void disconnect(void);
};
