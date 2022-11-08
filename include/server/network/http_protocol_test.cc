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

int main() {

  {
    network::HTTPProtocol http_protocol;

    http_protocol.request("POST", "/test.jpg");
    http_protocol.add_header("Host", "localhost:8000");
    http_protocol.add_header("User-Agent", "Mozilla/5.0 Chrome/99.99");
    http_protocol.add_header("Content-Type", "Text");
    http_protocol.add_header("Content-Length", "15");
    http_protocol.set_content("Hello, network!");

    auto generator = http_protocol.build();

    const auto p = generator.GenerateNext();
    if (!p) TEST_FAIL;
    if (const auto view = p->string_view();
        view != "POST /test.jpg HTTP/1.1\r\n"
                "Host: localhost:8000\r\n"
                "User-Agent: Mozilla/5.0 Chrome/99.99\r\n"
                "Content-Type: Text\r\n"
                "Content-Length: 15\r\n"
                "\r\n"
                "Hello, network!") TEST_FAIL;
  }

  { // Make response
    network::HTTPProtocol protocol;
    protocol.response(404, "FORBIDDEN");
    protocol.add_header("header", "value");
    protocol.set_content("Access denied");
    auto generator = protocol.build();
    auto packet = generator.GenerateNext();
    if (packet; packet->to_string() != "HTTP/1.1 404 FORBIDDEN\r\n"
                                       "header: value\r\n"
                                       "\r\n"
                                       "Access denied") TEST_FAIL;
  }

  {
    constexpr const char* response =
      "HTTP/1.1 403 FORBIDDEN\r\n"
      "Server: Apache\r\n"
      "Content-Type: text/html; charset=iso-8895-1\r\n"
      "Date: Sun, 6 Nov 2022 20:54:51 GMT\r\n"
      "\r\n"
      "<!DOCTYPE HTML PUBLIC \"-//IETF/DTD HTML 2.0//EN\">"
      "<h1>FORBIDDEN</h1>";

    network::HTTPProtocol parser;
    parser.parse(response);

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
