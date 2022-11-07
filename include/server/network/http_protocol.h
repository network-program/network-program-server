//
// Created by YongGyu Lee on 2022/10/31.
//

#ifndef SERVER_NETWORK_HTTP_PROTOCOL_H_
#define SERVER_NETWORK_HTTP_PROTOCOL_H_

#include "server/network/protocol.h"

namespace network {

template<size_t PacketSize>
class BasicHTTPProtocol :
  public BasicProtocol<PacketSize, BasicPacketGenerator<StringPacket>> {
 public:
  using base = BasicProtocol<PacketSize, BasicPacketGenerator<StringPacket>>;
  using base::packet_size;
  using generator = typename base::generator;
  using string_type = typename base::string_type;

  BasicHTTPProtocol() : base(": ", "\r\n") {}

  BasicHTTPProtocol& request(string_type request_type, string_type content) {
    if (!request_line_.empty()) {
      base::error("Request line already exists!. Existing request line will be overwritten");
    }
    request_line_ = request_type + " " + content + " " + http_version_;
    return *this;
  }

  generator build(std::stringstream ss = std::stringstream()) {
    ss << request_line_ << base::key_separator();
    return base::build(std::move(ss));
  }

  void parse(const string_type& str) {
    const auto p = str.find(base::key_separator(), 0);
    ParseResponseLine(str.substr(0, p));
    base::parse(str.substr(p + base::key_separator().size()));
  }

  NETWORK_NODISCARD int status_code() const { return status_code_; }
  NETWORK_NODISCARD string_type status_text() const { return status_text_; }
  NETWORK_NODISCARD string_type http_version() const { return http_version_; }

 private:
  void ParseResponseLine(string_type response_line) {
    status_code_ = -1;
    http_version_ = "";
    status_text_ = "";
    const string_type delimiter = " ";

    // Parse HTTP version
    auto p = response_line.find(delimiter);
    if (p == std::string::npos) return;
    http_version_ = response_line.substr(0, p);
    response_line.erase(0, p + delimiter.length());

    // Parse status code
    p = response_line.find(delimiter);
    if (p == std::string::npos) return;
    try {
      status_code_ = std::stoi(response_line.substr(0, p));
    } catch (...) {
      // N/A
    }
    response_line.erase(0, p + delimiter.length());

    status_text_ = std::move(response_line);
  }

  string_type request_line_;
  int status_code_ = -1;
  string_type status_text_;
  string_type http_version_ = "HTTP/1.1";
};

using HTTPProtocol = BasicHTTPProtocol<65535>;

} // namespace network

#endif // SERVER_NETWORK_HTTP_PROTOCOL_H_
