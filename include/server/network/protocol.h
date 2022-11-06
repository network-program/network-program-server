//
// Created by YongGyu Lee on 2022/10/14.
//

#ifndef SERVER_NETWORK_PROTOCOL_H_
#define SERVER_NETWORK_PROTOCOL_H_

#include <cassert>
#include <cstdint>
#include <cstdlib>
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

class BasicPacket {
 public:
  using string_type = std::string;
  using pointer = typename string_type::pointer;
  using const_pointer = typename string_type::const_pointer;
  using size_type = typename string_type::size_type;

  explicit BasicPacket(size_t buffer_size) : buffer_(buffer_size, '\0') {}

  template<typename T = const_pointer, std::enable_if_t<std::is_pointer_v<T>, int> = 0>
  NETWORK_NODISCARD T data(size_t index = 0) const { return reinterpret_cast<T>(buffer_.data() + index); }
  NETWORK_NODISCARD size_type size() const { return buffer_.size(); }
  NETWORK_NODISCARD size_type used_size() const { return write_idx; }

  NETWORK_NODISCARD std::string_view string_view() const { return data(); }
  NETWORK_NODISCARD string_type to_string() const { return data(); }

  BasicPacket& operator << (const string_type& str) {
    CheckOverflow(str.size());
    std::memcpy(data(write_idx), str.data(), str.size());
    write_idx += str.size();
    return *this;
  }

 protected:
  template<typename T = pointer, std::enable_if_t<std::is_pointer_v<T>, int> = 0>
  NETWORK_NODISCARD T data(size_t index = 0) { return reinterpret_cast<T>(buffer_.data() + index); }

  void CheckOverflow(size_t write_size) const {
    NETWORK_ASSERT(write_size + write_idx <= buffer_.size(), "Overflow while writing to packet");
  }

  std::string buffer_;
  size_t write_idx = 0;
};

class StringPacket : public BasicPacket {
  template<typename ...>
  struct always_false : std::false_type {};

 public:
  using base = BasicPacket;
  using string_type = base::string_type;
  using size_type = base::size_type;
  using base::operator<<;

  explicit StringPacket(size_t buffer_size) : base(buffer_size) {}

  template<typename T> NETWORK_NODISCARD T get(size_t index) const;
  template<> NETWORK_NODISCARD int get<int>(size_t index) const { return std::atoi(base::data(index)); }
  template<> NETWORK_NODISCARD long get<long>(size_t index) const { return std::atol(base::data(index)); }
  template<> NETWORK_NODISCARD long long get<long long>(size_t index) const { return std::atoll(base::data(index)); }

  template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
  BasicPacket& operator << (const T& value) {
    return (*this) << std::to_string(value);
  }
};

class BinaryPacket : public BasicPacket {
 public:
  using base = BasicPacket;
  using string_type = base::string_type;
  using size_type = base::size_type;
  using base::operator<<;

  explicit BinaryPacket(size_t buffer_size) : BasicPacket(buffer_size) {}

  template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
  NETWORK_NODISCARD T get(size_t index) const {
    return *base::data<const T*>(index);
  }

  template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
  BasicPacket& operator << (const T& value) {
    CheckOverflow(sizeof(T));
    (*base::data<T*>()) = value;
    write_idx += sizeof(T);
    return *this;
  }
};

template<typename Packet>
class BasicPacketGenerator {
 public:
  using packet = Packet;
  using string_type = typename packet::string_type;

  BasicPacketGenerator(uint32_t total_packet_num, size_t packet_size, string_type data)
    : data_(std::move(data)),
      max_packet_size_(packet_size),
      total_packet_num_(total_packet_num)
  {}

  BasicPacketGenerator(uint32_t max_packet_size, string_type data)
    : total_packet_num_(data.empty() ? 0 : ((data.size() - 1) / max_packet_size) + 1),
      data_(std::move(data)),
      max_packet_size_(max_packet_size)
  {}

  virtual std::optional<packet> GenerateNext() {
    if (write_offset_ >= data().size()) {
      return {};
    }

    const uint32_t packet_size = std::min(max_packet_size_, static_cast<uint32_t>(data().size()));
    packet p(packet_size);

    p << data().substr(write_offset_, packet_size);
    write_offset_ += packet_size;

    return p;
  }

  NETWORK_NODISCARD auto total_packet_num() const { return total_packet_num_; }
  NETWORK_NODISCARD auto generated_packet_num() const { return generated_packet_index_; }

 protected:
  NETWORK_NODISCARD string_type& data() { return data_; }
  NETWORK_NODISCARD const string_type& data() const { return data_; }
  uint32_t generated_packet_index_ = 0;
  const uint32_t total_packet_num_;

 private:
  typename string_type::size_type write_offset_ = 0;
  const uint32_t max_packet_size_;
  string_type data_;
};

template<size_t PacketSize, typename PacketGenerator>
class BasicProtocol {
 public:
  enum {
    packet_size = PacketSize
  };

  using string_type = std::string;
  using key_type = string_type;
  using value_type = string_type;
  using generator = PacketGenerator;

  using header_type = std::unordered_map<key_type, value_type>;
  using header_sequence_type = std::vector<header_type::iterator>;

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
//    if (const auto it = std::find(header().begin(), header().end(), header_type::value_type(key, value)); it != header().end()) {
    if (const auto it = header().find(key); it != header().end()) {
      std::stringstream ss;
      ss << "Key " << it->first << " already exists!"
         << " Existing key will be overwritten\n";
      error(ss.str());

      header_.emplace(std::forward<Key>(key), std::forward<Value>(value));
      auto prev_it = std::find(header_sequence_.begin(), header_sequence_.end(), it);
      std::swap(*prev_it, header_sequence_.back());
    } else {
      auto p = header_.emplace(std::forward<Key>(key), std::forward<Value>(value));
      header_sequence_.emplace_back(p.first);
    }
  }

   void set_content(string_type data) {
     if (!content_.empty()) {
       error("Content already exists! Existing content will be overwritten.\n");
     }

     content_ = std::move(data);
   }

  NETWORK_NODISCARD const header_type& header() const { return header_; }

  NETWORK_NODISCARD string_type& content() { return content_; }
  NETWORK_NODISCARD const string_type& content() const { return content_; }

  NETWORK_NODISCARD generator build(std::stringstream ss = std::stringstream()) const {
    for (const auto it : header_sequence_) {
      const auto& [key, value] = *it;
      ss << key << key_value_separator_ << value << key_separator_;
    }

    ss << key_separator_;
    ss << content_;

    return generator(packet_size, ss.str());
  }

  virtual void parse(const string_type& str) {
    if (!header_.empty())
      error("Header already exists! Existing values will be overwritten");
    if (!content_.empty())
      error("Content already exists! Existing values will be overwritten");
    clear();

    string_type::size_type pos_begin = 0;
    string_type::size_type pos_end = 0;
    while (pos_begin < str.size()) {
      pos_end = str.find(key_separator_, pos_begin);

      // Content
      if (pos_begin == pos_end)
        break;

      // Header
      const auto sep_pos = str.find(key_value_separator_, pos_begin);
      std::string key(str.begin() + pos_begin, str.begin() + sep_pos);
      std::string value(str.begin() + sep_pos + key_value_separator_.size(), str.begin() + pos_end);
      add_header(std::move(key), std::move(value));
      pos_begin = pos_end + key_separator_.size();
    }
    set_content(str.substr(std::min(pos_end + key_separator_.size(), str.size())));
  }

  void clear() {
    header_.clear();
    header_sequence_.clear();
    content_.clear();
  }

  NETWORK_NODISCARD bool empty() const {
    return header_.empty() && content_.empty();
  }

  NETWORK_NODISCARD const string_type& key_separator() const { return key_separator_; }
  NETWORK_NODISCARD const string_type& key_value_separator() const { return key_value_separator_; }

 protected:
  void error(const std::string& message) {
    std::cerr << message;
  }

 private:
  header_type header_;
  header_sequence_type header_sequence_;
  string_type content_;
  string_type key_value_separator_;
  string_type key_separator_;
};

} // namespace common

#endif // SERVER_NETWORK_PROTOCOL_H_
