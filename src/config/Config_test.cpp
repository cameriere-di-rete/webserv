#include "Config.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

// Helper to create a temporary config file
class TempConfigFile {
 public:
  TempConfigFile(const std::string& content) {
    path_ = "/tmp/test_config_XXXXXX.conf";
    // Create unique filename
    static int counter = 0;
    std::ostringstream oss;
    oss << "/tmp/test_config_" << counter++ << ".conf";
    path_ = oss.str();

    std::ofstream ofs(path_.c_str());
    ofs << content;
    ofs.close();
  }

  ~TempConfigFile() {
    std::remove(path_.c_str());
  }

  const std::string& path() const {
    return path_;
  }

 private:
  std::string path_;
};

TEST(ConfigLocationValidation, LocationWithBothCgiAndRedirectThrows) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /test {\n"
      "    cgi on;\n"
      "    redirect 301 /new-location;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(
      {
        try {
          cfg.getServers();
        } catch (const std::runtime_error& e) {
          std::string msg = e.what();
          EXPECT_NE(msg.find("cgi"), std::string::npos);
          EXPECT_NE(msg.find("redirect"), std::string::npos);
          throw;
        }
      },
      std::runtime_error);
}

TEST(ConfigLocationValidation, LocationWithOnlyCgiIsValid) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /cgi-bin {\n"
      "    cgi on;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_NO_THROW(cfg.getServers());
}

TEST(ConfigLocationValidation, LocationWithOnlyRedirectIsValid) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /old {\n"
      "    redirect 301 /new;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_NO_THROW(cfg.getServers());
}

TEST(ConfigLocationValidation, LocationWithNeitherCgiNorRedirectIsValid) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /static {\n"
      "    autoindex on;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_NO_THROW(cfg.getServers());
}

TEST(ConfigLocationValidation, LocationWithCgiOffAndRedirectIsValid) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /test {\n"
      "    cgi off;\n"
      "    redirect 302 /other;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_NO_THROW(cfg.getServers());
}
