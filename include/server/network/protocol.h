//
// Created by YongGyu Lee on 2022/10/14.
//

#ifndef SERVER_NETWORK_PROTOCOL_H_
#define SERVER_NETWORK_PROTOCOL_H_

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <optional>

#if __cplusplus >= 201703L
#define NETWORK_NODISCARD [[nodiscard]]
#else
#define COMMON_NODISCARD
#endif

#define NETWORK_ASSERT(expr, msg) \
  assert(((void)msg, (expr)))

namespace network {

class BasicPacketGenerator {
 public:
//    |  N  |               | 32-bytes
//    |             | 32-bytes
//    |             | 32-bytes
//    |             | 32-bytes
//    |             | 32-bytes
//    |             | 32-bytes

  class Packet {
   public:
    enum {
      packet_size_byte    = sizeof(uint32_t),
      packet_size_offset  = 0,
      sequence_byte       = sizeof(uint32_t),
      current_sequence_byte = packet_size_offset + packet_size_byte,
      total_sequence_offset = current_sequence_byte + sequence_byte,
      packet_header_byte    = total_sequence_offset + sequence_byte,
    };

    NETWORK_NODISCARD uint32_t current_sequence() const { return get<uint32_t>(current_sequence_byte); }
    NETWORK_NODISCARD uint32_t total_sequence() const { return get<uint32_t>(total_sequence_offset); }
    NETWORK_NODISCARD std::string_view buffer() const { return buffer_; }
    NETWORK_NODISCARD std::string_view content() const { return buffer_.data() + packet_header_byte; }
    NETWORK_NODISCARD size_t size() const { return buffer_.size(); }

   private:
    friend class BasicPacketGenerator;
    Packet(size_t buffer_size) : buffer_(buffer_size, '\0') {}

    template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
    NETWORK_NODISCARD const T& get(size_t index) const {
      return *reinterpret_cast<const T*>(buffer_.data() + index);
    }
    template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
    NETWORK_NODISCARD T& get(size_t index) {
      return *const_cast<T*>(reinterpret_cast<const T*>(buffer_.data() + index));
    }

    template<typename T, std::enable_if_t<std::is_pointer_v<T>, int> = 0>
    NETWORK_NODISCARD T get(size_t index) const {
      return reinterpret_cast<const T>(buffer_.data() + index);
    }
    template<typename T, std::enable_if_t<std::is_pointer_v<T>, int> = 0>
    NETWORK_NODISCARD T get(size_t index) {
      return const_cast<T>(reinterpret_cast<const T>(buffer_.data() + index));
    }

    template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
    Packet& operator << (const T& value) {
      get<T>(write_idx) = value;
      write_idx += sizeof(T);
      return *this;
    }

    Packet& operator << (const std::string& str) {
      NETWORK_ASSERT(str.size() + write_idx <= buffer_.size(), "Overflow while writing to packet");
      std::memcpy(get<char *>(write_idx), str.data(), str.size());
      write_idx += str.size();
      return *this;
    }

    std::string buffer_;
    size_t write_idx = 0;
  };

  using packet = Packet;
  using string_type = std::string;

  BasicPacketGenerator(string_type data, uint32_t packet_size)
    : data_(std::move(data)),
      max_packet_size_(packet_size),
      total_packet_num_(data_.empty() ? 0 :
                        ((data_.size() - 1) / (max_packet_size_ - packet::packet_header_byte)) + 1)
  {
    NETWORK_ASSERT(max_packet_size_ > packet::packet_header_byte,
                   "Packet size must be greater than packet header size");
  }

  std::optional<packet> GenerateNext() {
    if (write_offset_ >= data_.size()) {
      return {};
    }

    const uint32_t packet_header_size = packet::packet_header_byte;
    const uint32_t packet_size = std::min(max_packet_size_, static_cast<uint32_t>(data_.size()) + packet_header_size);
    const uint32_t packet_data_size = packet_size - packet_header_size;
    Packet packet(packet_size);

    packet << packet_size;
    packet << ++generated_packet_index_;
    packet << total_packet_num_;

#ifndef NDEBUG
    NETWORK_ASSERT(packet.write_idx == sizeof(uint32_t) * 3, "Invalid write index");
#endif
    packet << data_.substr(write_offset_, packet_data_size);
    write_offset_ += packet_data_size;

    return packet;
  }

  NETWORK_NODISCARD auto total_packet_num() const { return total_packet_num_; }
  NETWORK_NODISCARD auto generated_packet_num() const { return generated_packet_index_; }

 private:
  string_type data_;
  typename string_type::size_type write_offset_ = 0;
  const uint32_t max_packet_size_;
  uint32_t generated_packet_index_ = 0;
  const uint32_t total_packet_num_;
};

template<size_t PacketSize>
class BasicProtocol {
 public:
  enum {
    packet_size = PacketSize
  };

  using string_type = std::string;
  using key_type = string_type;
  using value_type = string_type;
//  using header_type = std::unordered_map<key_type, value_type>;
  using header_type = std::vector<std::pair<key_type, value_type>>;

  BasicProtocol(string_type key_value_separator, string_type key_separator)
    : key_value_separator_(std::move(key_value_separator)),
      key_separator_(std::move(key_separator))
  {}

  template<typename Key, typename Value>
  std::enable_if_t<
    std::conjunction_v<
      std::is_constructible<string_type, Key>,
      std::is_constructible<string_type, Value>
    >
  >
  add_header(Key&& key, Value&& value) {
    if (const auto it = std::find(header().begin(), header().end(), header_type::value_type(key, value)); it != header().end()) {
//    if (const auto it = header().find(key); it != header().end()) {
      std::stringstream ss;
      ss << "Key " << it->first << " already exists!"
         << " Existing key will be overwritten\n";
      error(ss.str());
    }
    header_.emplace_back(std::forward<Key>(key), std::forward<Value>(value));
  }

   void set_content(string_type data) {
     if (!content_.empty()) {
       error("Content already exists! Existing content will be overwritten.\n");
     }

     content_ = std::move(data);
   }

  NETWORK_NODISCARD header_type& header() { return header_; }
  NETWORK_NODISCARD const header_type& header() const { return header_; }

  NETWORK_NODISCARD string_type& content() { return content_; }
  NETWORK_NODISCARD const string_type& content() const { return content_; }

  template<typename Generator = BasicPacketGenerator>
  NETWORK_NODISCARD Generator build() const {
    std::stringstream ss;

    for (const auto& [key, value] : header()) {
      ss << key << key_value_separator_ << value << key_separator_;
    }
    ss << key_separator_;
    ss << content_;

    return Generator(ss.str(), packet_size);
  }

  void clear() {
    header_.clear();
    content_.clear();
  }

 private:
  void error(const std::string& message) {
    std::cerr << message;
  }

  header_type header_;
  string_type content_;
  string_type key_value_separator_;
  string_type key_separator_;
};

using Protocol = BasicProtocol<1'000'000>;

Protocol MakeProtocol() {
  return {": ", "\r\n"};
}

template<size_t PacketSize>
class BasicHTTPProtocol :
  public BasicProtocol<PacketSize> {
 public:
  using base = BasicProtocol<PacketSize>;
  using base::packet_size;
  using string_type = typename base::string_type;

  BasicHTTPProtocol() : base(": ", "\r\n") {}

  BasicHTTPProtocol& request(string_type request_line) {
    if (!request_line_.empty()) {
      base::error("Request line already exists!. Existing request line will be overwritten");
    }
    request_line_ = std::move(request_line);
    return *this;
  }


 private:
  string_type request_line_;
};

} // namespace common

#endif // SERVER_NETWORK_PROTOCOL_H_
