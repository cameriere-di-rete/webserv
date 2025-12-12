#include "Config.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

#include "core/Server.hpp"

// Helper to create a temporary config file
class TempConfigFile {
 public:
  TempConfigFile(const std::string& content) {
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

// ==================== BASIC CONFIG STRUCTURE TESTS ====================

TEST(ConfigBasic, EmptyConfigThrows) {
  std::string config = "";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

TEST(ConfigBasic, NoServerBlockThrows) {
  std::string config = "max_request_body 1024;\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

TEST(ConfigBasic, NonServerTopLevelBlockThrows) {
  std::string config =
      "location /test {\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

TEST(ConfigBasic, MinimalValidConfig) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers;
  EXPECT_NO_THROW(servers = cfg.getServers());
  EXPECT_EQ(servers.size(), 1u);
  EXPECT_EQ(servers[0].port, 8080);
  EXPECT_EQ(servers[0].root, "/var/www");
}

TEST(ConfigBasic, MultipleServerBlocks) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www1;\n"
      "}\n"
      "server {\n"
      "  listen 8081;\n"
      "  root /var/www2;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers.size(), 2u);
  EXPECT_EQ(servers[0].port, 8080);
  EXPECT_EQ(servers[1].port, 8081);
}

TEST(ConfigBasic, CommentsAreIgnored) {
  std::string config =
      "# This is a comment\n"
      "server {\n"
      "  listen 8080; # inline comment\n"
      "  root /var/www;\n"
      "  # another comment\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers;
  EXPECT_NO_THROW(servers = cfg.getServers());
  EXPECT_EQ(servers.size(), 1u);
}

// ==================== FILE HANDLING TESTS ====================

TEST(ConfigFile, NonexistentFileThrows) {
  Config cfg;
  EXPECT_THROW(cfg.parseFile("/nonexistent/path/to/config.conf"),
               std::runtime_error);
}

// ==================== LISTEN DIRECTIVE TESTS ====================

TEST(ConfigListen, MissingListenThrows) {
  std::string config =
      "server {\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

TEST(ConfigListen, PortOnly) {
  std::string config =
      "server {\n"
      "  listen 9000;\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].port, 9000);
}

TEST(ConfigListen, HostAndPort) {
  std::string config =
      "server {\n"
      "  listen 127.0.0.1:8080;\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].port, 8080);
}

TEST(ConfigListen, InvalidPortZeroThrows) {
  std::string config =
      "server {\n"
      "  listen 0;\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

TEST(ConfigListen, InvalidPortTooHighThrows) {
  std::string config =
      "server {\n"
      "  listen 65536;\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

TEST(ConfigListen, InvalidPortNonNumericThrows) {
  std::string config =
      "server {\n"
      "  listen abc;\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

TEST(ConfigListen, InvalidHostThrows) {
  std::string config =
      "server {\n"
      "  listen invalid.host:8080;\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

TEST(ConfigListen, MissingArgumentThrows) {
  std::string config =
      "server {\n"
      "  listen;\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

// ==================== ROOT DIRECTIVE TESTS ====================

TEST(ConfigRoot, MissingRootThrows) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

TEST(ConfigRoot, ValidRootPath) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www/html;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].root, "/var/www/html");
}

// ==================== INDEX DIRECTIVE TESTS ====================

TEST(ConfigIndex, SingleIndexFile) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  index index.html;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].index.size(), 1u);
  EXPECT_NE(servers[0].index.find("index.html"), servers[0].index.end());
}

// ==================== AUTOINDEX DIRECTIVE TESTS ====================

TEST(ConfigAutoindex, AutoindexOn) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  autoindex on;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_TRUE(servers[0].autoindex);
}

TEST(ConfigAutoindex, AutoindexOff) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  autoindex off;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_FALSE(servers[0].autoindex);
}

TEST(ConfigAutoindex, InvalidValueThrows) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  autoindex yes;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

// ==================== ALLOW_METHODS DIRECTIVE TESTS ====================

TEST(ConfigAllowMethods, SingleMethod) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  allow_methods GET;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].allow_methods.size(), 1u);
  EXPECT_NE(servers[0].allow_methods.find(http::GET),
            servers[0].allow_methods.end());
}

TEST(ConfigAllowMethods, MultipleMethods) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  allow_methods GET POST DELETE;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].allow_methods.size(), 3u);
  EXPECT_NE(servers[0].allow_methods.find(http::GET),
            servers[0].allow_methods.end());
  EXPECT_NE(servers[0].allow_methods.find(http::POST),
            servers[0].allow_methods.end());
  EXPECT_NE(servers[0].allow_methods.find(http::DELETE),
            servers[0].allow_methods.end());
}

TEST(ConfigAllowMethods, InvalidMethodThrows) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  allow_methods INVALID;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

// ==================== ERROR_PAGE DIRECTIVE TESTS ====================

TEST(ConfigErrorPage, SingleErrorPage) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  error_page 404 /404.html;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].error_page.size(), 1u);
  EXPECT_EQ(servers[0].error_page[http::S_404_NOT_FOUND], "/404.html");
}

TEST(ConfigErrorPage, MultipleStatusCodesOnePage) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  error_page 500 502 503 /50x.html;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].error_page.size(), 3u);
  EXPECT_EQ(servers[0].error_page[http::S_500_INTERNAL_SERVER_ERROR],
            "/50x.html");
  EXPECT_EQ(servers[0].error_page[http::S_502_BAD_GATEWAY], "/50x.html");
  EXPECT_EQ(servers[0].error_page[http::S_503_SERVICE_UNAVAILABLE],
            "/50x.html");
}

TEST(ConfigErrorPage, InvalidStatusCodeThrows) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  error_page 200 /success.html;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

TEST(ConfigErrorPage, MissingArgsThrows) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  error_page 404;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

// ==================== MAX_REQUEST_BODY DIRECTIVE TESTS ====================

TEST(ConfigMaxRequestBody, ValidValue) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  max_request_body 1048576;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].max_request_body, 1048576u);
}

TEST(ConfigMaxRequestBody, InvalidValueThrows) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  max_request_body -1;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

TEST(ConfigMaxRequestBody, GlobalMaxRequestBody) {
  std::string config =
      "max_request_body 2048;\n"
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].max_request_body, 2048u);
}

TEST(ConfigMaxRequestBody, ServerOverridesGlobal) {
  std::string config =
      "max_request_body 2048;\n"
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  max_request_body 4096;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].max_request_body, 4096u);
}

// ==================== GLOBAL ERROR_PAGE TESTS ====================

TEST(ConfigGlobalErrorPage, GlobalErrorPageApplied) {
  std::string config =
      "error_page 500 /global_error.html;\n"
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].error_page[http::S_500_INTERNAL_SERVER_ERROR],
            "/global_error.html");
}

// ==================== LOCATION BLOCK TESTS ====================

TEST(ConfigLocation, BasicLocation) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /api {\n"
      "    root /var/api;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].locations.size(), 1u);
  EXPECT_EQ(servers[0].locations["/api"].root, "/var/api");
}

TEST(ConfigLocation, MultipleLocations) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /api {\n"
      "    root /var/api;\n"
      "  }\n"
      "  location /static {\n"
      "    root /var/static;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].locations.size(), 2u);
}

TEST(ConfigLocation, LocationWithIndex) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /docs {\n"
      "    index readme.html;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_GE(servers[0].locations["/docs"].index.size(), 1u);
}

TEST(ConfigLocation, LocationWithAutoindex) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /files {\n"
      "    autoindex on;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_TRUE(servers[0].locations["/files"].autoindex);
}

TEST(ConfigLocation, LocationWithAllowMethods) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /upload {\n"
      "    allow_methods POST PUT;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].locations["/upload"].allow_methods.size(), 2u);
}

TEST(ConfigLocation, LocationWithErrorPage) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /api {\n"
      "    error_page 500 /api_error.html;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0]
                .locations["/api"]
                .error_page[http::S_500_INTERNAL_SERVER_ERROR],
            "/api_error.html");
}

// ==================== REDIRECT DIRECTIVE TESTS ====================

TEST(ConfigRedirect, ValidRedirect301) {
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

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].locations["/old"].redirect_code,
            http::S_301_MOVED_PERMANENTLY);
  EXPECT_EQ(servers[0].locations["/old"].redirect_location, "/new");
}

TEST(ConfigRedirect, ValidRedirect302) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /temp {\n"
      "    redirect 302 /temporary;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].locations["/temp"].redirect_code, http::S_302_FOUND);
}

TEST(ConfigRedirect, ValidRedirect307) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /temp {\n"
      "    redirect 307 /temporary;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].locations["/temp"].redirect_code,
            http::S_307_TEMPORARY_REDIRECT);
}

TEST(ConfigRedirect, ValidRedirect308) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /perm {\n"
      "    redirect 308 /permanent;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].locations["/perm"].redirect_code,
            http::S_308_PERMANENT_REDIRECT);
}

TEST(ConfigRedirect, InvalidRedirectCodeThrows) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /bad {\n"
      "    redirect 200 /success;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

TEST(ConfigRedirect, InvalidRedirectCode404Throws) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /bad {\n"
      "    redirect 404 /notfound;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

TEST(ConfigRedirect, MissingArgumentsThrows) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /bad {\n"
      "    redirect 301;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

// ==================== CGI ROOT TESTS ====================

TEST(ConfigCgiRoot, CgiRootSet) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /cgi-bin {\n"
      "    cgi_root /var/www/cgi;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].locations["/cgi-bin"].cgi_root, "/var/www/cgi");
  EXPECT_FALSE(servers[0].locations["/cgi-bin"].cgi_root.empty());
}

TEST(ConfigCgiRoot, CgiRootNotSet) {
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

  std::vector<Server> servers = cfg.getServers();
  EXPECT_TRUE(servers[0].locations["/static"].cgi_root.empty());
}

TEST(ConfigCgiRoot, CgiRootMissingValueThrows) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /cgi {\n"
      "    cgi_root;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

// ==================== CGI AND REDIRECT VALIDATION TESTS ====================

TEST(ConfigLocationValidation, LocationWithBothCgiRootAndRedirectThrows) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /test {\n"
      "    cgi_root /var/www/cgi;\n"
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
          EXPECT_NE(msg.find("cgi_root"), std::string::npos);
          EXPECT_NE(msg.find("redirect"), std::string::npos);
          throw;
        }
      },
      std::runtime_error);
}

TEST(ConfigLocationValidation, LocationWithOnlyCgiRootIsValid) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /cgi-bin {\n"
      "    cgi_root /var/www/cgi;\n"
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

TEST(ConfigLocationValidation, LocationWithNeitherCgiRootNorRedirectIsValid) {
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
// ==================== UNRECOGNIZED DIRECTIVE TESTS ====================

TEST(ConfigUnrecognized, UnrecognizedGlobalDirectiveThrows) {
  std::string config =
      "unknown_directive value;\n"
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

TEST(ConfigUnrecognized, UnrecognizedServerDirectiveThrows) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  unknown_directive value;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

TEST(ConfigUnrecognized, UnrecognizedLocationDirectiveThrows) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /test {\n"
      "    unknown_directive value;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

// ==================== SYNTAX ERROR TESTS ====================

TEST(ConfigSyntax, MissingSemicolonCausesValidationError) {
  std::string config =
      "server {\n"
      "  listen 8080\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  // Missing semicolon causes 'listen' to absorb extra tokens as arguments,
  // which triggers a validation error (expects exactly 1 argument)
  EXPECT_THROW(cfg.getServers(), std::runtime_error);
}

TEST(ConfigSyntax, MissingClosingBraceThrows) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n";

  TempConfigFile tmpFile(config);
  Config cfg;

  EXPECT_THROW(cfg.parseFile(tmpFile.path()), std::runtime_error);
}

TEST(ConfigSyntax, MissingOpeningBraceThrows) {
  std::string config =
      "server\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;

  EXPECT_THROW(cfg.parseFile(tmpFile.path()), std::runtime_error);
}

TEST(ConfigSyntax, LocationMissingPathThrows) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location {\n"
      "    autoindex on;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;

  EXPECT_THROW(cfg.parseFile(tmpFile.path()), std::runtime_error);
}

// ==================== COPY AND ASSIGNMENT TESTS ====================

TEST(ConfigCopy, CopyConstructor) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg1;
  cfg1.parseFile(tmpFile.path());

  Config cfg2(cfg1);
  std::vector<Server> servers = cfg2.getServers();
  EXPECT_EQ(servers.size(), 1u);
  EXPECT_EQ(servers[0].port, 8080);
}

TEST(ConfigCopy, AssignmentOperator) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg1;
  cfg1.parseFile(tmpFile.path());

  Config cfg2;
  cfg2 = cfg1;
  std::vector<Server> servers = cfg2.getServers();
  EXPECT_EQ(servers.size(), 1u);
  EXPECT_EQ(servers[0].port, 8080);
}

// ==================== EDGE CASE TESTS ====================

TEST(ConfigEdgeCases, WhitespaceHandling) {
  std::string config =
      "   server   {   \n"
      "     listen    8080   ;   \n"
      "     root   /var/www   ;   \n"
      "   }   \n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers.size(), 1u);
}

TEST(ConfigEdgeCases, TabsAndNewlines) {
  std::string config =
      "\tserver\t{\n"
      "\t\tlisten\t8080;\n"
      "\t\troot\t/var/www;\n"
      "\t}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers.size(), 1u);
}

TEST(ConfigEdgeCases, ValidPortBoundary1) {
  std::string config =
      "server {\n"
      "  listen 1;\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].port, 1);
}

TEST(ConfigEdgeCases, ValidPortBoundary65535) {
  std::string config =
      "server {\n"
      "  listen 65535;\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].port, 65535);
}

TEST(ConfigEdgeCases, ComplexConfiguration) {
  std::string config =
      "max_request_body 1024;\n"
      "error_page 500 502 /50x.html;\n"
      "server {\n"
      "  listen 127.0.0.1:8080;\n"
      "  root /var/www;\n"
      "  index index.html;\n"
      "  autoindex off;\n"
      "  allow_methods GET POST;\n"
      "  error_page 404 /404.html;\n"
      "  max_request_body 2048;\n"
      "  location /api {\n"
      "    root /var/api;\n"
      "    allow_methods GET POST PUT DELETE;\n"
      "  }\n"
      "  location /uploads {\n"
      "    autoindex on;\n"
      "  }\n"
      "  location /old {\n"
      "    redirect 301 /new;\n"
      "  }\n"
      "  location /cgi-bin {\n"
      "    cgi_root /var/cgi-bin;\n"
      "  }\n"
      "}\n"
      "server {\n"
      "  listen 8081;\n"
      "  root /var/www2;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers.size(), 2u);
  EXPECT_EQ(servers[0].port, 8080);
  EXPECT_EQ(servers[0].max_request_body, 2048u);
  EXPECT_EQ(servers[0].locations.size(), 4u);
  EXPECT_EQ(servers[1].port, 8081);
  EXPECT_EQ(servers[1].max_request_body, 1024u);  // inherited from global
}

// ==================== MAX_REQUEST_BODY INHERITANCE TESTS ====================

TEST(MaxRequestBodyInheritance, LocationInheritsFromServer) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  max_request_body 4096;\n"
      "  location /api {\n"
      "    root /var/api;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].max_request_body, 4096u);

  // Location should inherit max_request_body from server
  Location loc = servers[0].matchLocation("/api/test");
  EXPECT_EQ(loc.max_request_body, 4096u);
}

TEST(MaxRequestBodyInheritance, LocationOverridesServer) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  max_request_body 4096;\n"
      "  location /uploads {\n"
      "    root /var/uploads;\n"
      "    max_request_body 10485760;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();
  EXPECT_EQ(servers[0].max_request_body, 4096u);

  // Location with explicit max_request_body should use its own value
  Location loc = servers[0].matchLocation("/uploads/file.txt");
  EXPECT_EQ(loc.max_request_body, 10485760u);
}

TEST(MaxRequestBodyInheritance, DefaultLocationInheritsFromServer) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  max_request_body 2048;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();

  // No matching location, should use server's max_request_body
  Location loc = servers[0].matchLocation("/nonexistent/path");
  EXPECT_EQ(loc.max_request_body, 2048u);
}

TEST(MaxRequestBodyInheritance, GlobalToServerToLocation) {
  std::string config =
      "max_request_body 1024;\n"
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  location /api {\n"
      "    root /var/api;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();

  // Server inherits from global
  EXPECT_EQ(servers[0].max_request_body, 1024u);

  // Location inherits from server (which inherited from global)
  Location loc = servers[0].matchLocation("/api/endpoint");
  EXPECT_EQ(loc.max_request_body, 1024u);
}

TEST(MaxRequestBodyInheritance, UnsetMeansNoLimit) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();

  // No max_request_body set anywhere, should be DEFAULT (4096)
  EXPECT_EQ(servers[0].max_request_body, DEFAULT_MAX_REQUEST_BODY);

  Location loc = servers[0].matchLocation("/");
  EXPECT_EQ(loc.max_request_body, DEFAULT_MAX_REQUEST_BODY);
}

TEST(MaxRequestBodyInheritance, MultipleLocationsWithDifferentLimits) {
  std::string config =
      "server {\n"
      "  listen 8080;\n"
      "  root /var/www;\n"
      "  max_request_body 4096;\n"
      "  location /small {\n"
      "    root /var/small;\n"
      "    max_request_body 1024;\n"
      "  }\n"
      "  location /large {\n"
      "    root /var/large;\n"
      "    max_request_body 104857600;\n"
      "  }\n"
      "  location /default {\n"
      "    root /var/default;\n"
      "  }\n"
      "}\n";

  TempConfigFile tmpFile(config);
  Config cfg;
  cfg.parseFile(tmpFile.path());

  std::vector<Server> servers = cfg.getServers();

  // Each location should have its correct limit
  Location smallLoc = servers[0].matchLocation("/small/file");
  EXPECT_EQ(smallLoc.max_request_body, 1024u);

  Location largeLoc = servers[0].matchLocation("/large/file");
  EXPECT_EQ(largeLoc.max_request_body, 104857600u);

  // Location without explicit max_request_body inherits from server
  Location defaultLoc = servers[0].matchLocation("/default/file");
  EXPECT_EQ(defaultLoc.max_request_body, 4096u);
}
