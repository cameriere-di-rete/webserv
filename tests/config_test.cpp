#include <gtest/gtest.h>

#include "Config.hpp"

TEST(ConfigParse, TestConfigFile) {
  Config cfg;
  cfg.parseFile(std::string("../conf/test.conf"));
  std::vector<Server> servers = cfg.getServers();
  EXPECT_GT(servers.size(), 0u);
  Server s = servers[0];
  // from test.conf expect port 8080 and root ./www
  EXPECT_EQ(s.port, 8080);
  EXPECT_NE(s.root.find("www"), std::string::npos);
  // location /ciaone should exist and be redirect
  auto it = s.locations.find("/ciaone");
  EXPECT_NE(it, s.locations.end());
  EXPECT_TRUE(http::isRedirect(it->second.redirect_code));
}
