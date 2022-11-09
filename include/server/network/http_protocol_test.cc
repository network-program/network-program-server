//
// Created by YongGyu Lee on 2022/10/14.
//

#include "server/network/http_protocol.h"

#include <iostream>

#define TEST_FAIL               \
do {                            \
  std::cerr                     \
      << "Test failed at "      \
      << __FILE__ << ", line "  \
      << __LINE__ << '\n';      \
  std::terminate();             \
} while(false)

#define TEST_PASS               \
do {                            \
  std::cerr                     \
      << "Test passed at line " \
      << __LINE__ << '\n';      \
} while (false)

constexpr const char* kRequest = "POST /test.jpg HTTP/1.1\r\n"
                                 "Host: localhost:8000\r\n"
                                 "User-Agent: Mozilla/5.0 Chrome/99.99\r\n"
                                 "Content-Type: Text\r\n"
                                 "\r\n"
                                 "Hello, network!";

constexpr const char* kResponse = "HTTP/1.1 403 FORBIDDEN\r\n"
                                  "Server: Apache\r\n"
                                  "Content-Type: text/html; charset=iso-8895-1\r\n"
                                  "Date: Sun, 6 Nov 2022 20:54:51 GMT\r\n"
                                  "Content-Length: 67\r\n"
                                  "\r\n"
                                  "<!DOCTYPE HTML PUBLIC \"-//IETF/DTD HTML 2.0//EN\">"
                                  "<h1>FORBIDDEN</h1>";


int main() {

  {
    network::HTTPProtocol http_protocol;

    http_protocol.request("POST", "/test.jpg");
    http_protocol.add_header("Host", "localhost:8000");
    http_protocol.add_header("User-Agent", "Mozilla/5.0 Chrome/99.99");
    http_protocol.set_content("Hello, network!", "Text");

    auto generator = http_protocol.build();

    const auto p = generator.GenerateNext();
    if (!p) TEST_FAIL;
    if (const auto view = p->string_view(); view != kRequest) TEST_FAIL;
  }

  auto check_key_value = [](const auto& map, const auto& key, const auto& value) {
    const auto it = map.find(key);
    if (it == map.end()) return false;
    if (it->second != value) return false;
    return true;
  };

  { // Parse request
    network::HTTPProtocol parser;
    if (const auto b = parser.parse(kRequest); !b) TEST_FAIL; else TEST_PASS;
    if (parser.http_method() != "POST") TEST_FAIL; else TEST_PASS;
    if (parser.request_target() != "/test.jpg") TEST_FAIL; else TEST_PASS;
    if (parser.http_version() != "HTTP/1.1") TEST_FAIL; else TEST_PASS;

    const auto& header = parser.header();
    if (!check_key_value(header, "Host", "localhost:8000")) TEST_FAIL; else TEST_PASS;
    if (!check_key_value(header, "User-Agent", "Mozilla/5.0 Chrome/99.99")) TEST_FAIL; else TEST_PASS;
    if (!check_key_value(header, "Content-Type", "Text")) TEST_FAIL; else TEST_PASS;

    if (const auto b = parser.parse("POST /key.txt HTTP1.1\r\n\r\n"); !b) TEST_FAIL; else TEST_PASS;
  }

  { // Parse invalid request
    network::HTTPProtocol parser;

    if (const auto b = parser.parse("You are such a moron"); b) TEST_FAIL; else TEST_PASS;
    if (const auto b = parser.parse("POST /key.txt HTTP1.1\r\n"); b) TEST_FAIL; else TEST_PASS;
    if (const auto b = parser.parse("POST /key.txt HTTP1.1\r\nheader:key"); b) TEST_FAIL; else TEST_PASS;
    if (const auto b = parser.parse("POST /key.txt HTTP1.1\r\nheader:key\r\n"); b) TEST_FAIL; else TEST_PASS;
  }

  { // Make response
    network::HTTPProtocol protocol;
    protocol.response(404, "FORBIDDEN");
    protocol.add_header("header", "value");
    protocol.set_content("Access denied", "Text");
    auto generator = protocol.build();
    auto packet = generator.GenerateNext();
    if (packet->to_string() != "HTTP/1.1 404 FORBIDDEN\r\n"
                               "header: value\r\n"
                               "Content-Type: Text\r\n"
                               "\r\n"
                               "Access denied") TEST_FAIL;
  }

  {
    network::HTTPProtocol parser;
    parser.parse(kResponse);

    if (parser.status_code() != 403) TEST_FAIL;
    if (parser.http_version() != "HTTP/1.1") TEST_FAIL;
    if (parser.status_text() != "FORBIDDEN") TEST_FAIL;

    auto header = parser.header();
    network::HTTPProtocol::header_type::const_iterator it;

    if (it = header.find("Server"); it == header.end()) TEST_FAIL;
    if (it->second != "Apache") TEST_FAIL;
    if (it = header.find("Content-Type"); it == header.end()) TEST_FAIL;
    if (it->second != "text/html; charset=iso-8895-1") TEST_FAIL;
    if (it = header.find("Date"); it == header.end()) TEST_FAIL;
    if (it->second != "Sun, 6 Nov 2022 20:54:51 GMT") TEST_FAIL;

    if (parser.content() != "<!DOCTYPE HTML PUBLIC \"-//IETF/DTD HTML 2.0//EN\">"
                            "<h1>FORBIDDEN</h1>") TEST_FAIL;
  }

  return EXIT_SUCCESS;
}
