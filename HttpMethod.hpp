#pragma once

#include <string>

namespace http {
enum Method { GET, POST, PUT, DELETE, HEAD };

std::string methodToString(Method m);
Method stringToMethod(const std::string &s);

} // namespace http
