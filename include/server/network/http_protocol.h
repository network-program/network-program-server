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
  static constexpr const char* kHTTP_1_1 = "HTTP/1.1";
 public:
  using base = BasicProtocol<PacketSize, BasicPacketGenerator<StringPacket>>;
  using base::packet_size;
  using generator = typename base::generator;
  using string_type = typename base::string_type;

  BasicHTTPProtocol() : base(": ", "\r\n") {}

  BasicHTTPProtocol& request(string_type http_method, string_type request_target) {
    if (!start_line_.empty()) {
      base::error("Start line already exists!. Existing start line will be overwritten");
    }
    http_method_ = http_method;
    request_target_ = request_target;
    start_line_ = http_method + " " + request_target + " " + http_version_;
    return *this;
  }

  BasicHTTPProtocol& response(int status_code, string_type status_text = "") {
    if (!start_line_.empty()) {
      base::error("Start line already exists!. Existing start line will be overwritten");
    }
    if (http_version_ != kHTTP_1_1) {
      base::error("HTTP version must be HTTP/1.1! Overriding to HTTP/1.1");
      http_version_ = kHTTP_1_1;
    }
    status_code_ = status_code;
    status_text_ = status_text;

    start_line_ = http_version_ + " " + std::to_string(status_code) + " " + status_text;
    return *this;
  }

  generator build(std::stringstream ss = std::stringstream()) {
    ss << start_line_ << base::key_separator();
    return base::build(std::move(ss));
  }

  bool parse(const string_type& str) override {
    // Parse request line
    const auto p = str.find(base::key_separator(), 0);
    auto b = ParseStartLine(str.substr(0, p));
    if (!b) return false;

    // Parse header & content
    return base::parse(str.substr(p + base::key_separator().size()));
  }

  NETWORK_NODISCARD int status_code() const { return status_code_; }
  NETWORK_NODISCARD const string_type& status_text()    const { return status_text_;    }
  NETWORK_NODISCARD const string_type& http_version()   const { return http_version_;   }
  NETWORK_NODISCARD const string_type& http_method()    const { return http_method_;    }
  NETWORK_NODISCARD const string_type& request_target() const { return request_target_; }

 private:
  bool ParseStartLine(string_type start_line) {
    const string_type delimiter = " ";
    std::vector<string_type> tokens;

    ClearStartLine();
    start_line_ = start_line;

    typename string_type::size_type p = 0;
    while (true) {
      const auto p2 = start_line.find(delimiter, p);
      tokens.emplace_back(start_line.substr(p, p2 - p));
      if (p2 == string_type::npos)
        break;
      p = p2 + delimiter.size();
    }

    if (tokens.size() < 3) {
      base::error("Invalid start line '", start_line, "'!");
      return false;
    }

    // Response
    if (tokens[0].substr(0, 4) == "HTTP") {
      int status_code;
      try {
        status_code = std::stoi(tokens[1]);
      } catch (...) {
        base::error("Invalid status code '", tokens[1], "'!");
        return false;
      }

      http_version_ = std::move(tokens[0]);
      status_code_ = status_code;
      for (int i = 2; i < tokens.size(); ++i) {
        status_text_ += std::move(tokens[i]);
      }
    } else { // Request
      if (tokens.size() != 3) {
        base::error("Invalid request start line '", start_line, "'!");
        return false;
      }
      http_method_ = tokens[0];
      request_target_ = tokens[1];
      http_version_ = tokens[2];
    }

    return true;
  }

  void ClearStartLine() {
    start_line_.clear();
    status_code_ = -1;
    status_text_.clear();
    http_version_ = kHTTP_1_1;
    http_method_.clear();
    request_target_.clear();
  }

  string_type start_line_;
  int status_code_ = -1;
  string_type status_text_;
  string_type http_version_ = kHTTP_1_1;
  string_type http_method_;
  string_type request_target_;
};

using HTTPProtocol = BasicHTTPProtocol<65535>;

} // namespace network

#endif // SERVER_NETWORK_HTTP_PROTOCOL_H_
