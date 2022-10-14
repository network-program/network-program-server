//
// Created by YongGyu Lee on 2022/10/14.
//

#ifndef SERVER_NETWORK_PROTOCOL_H_
#define SERVER_NETWORK_PROTOCOL_H_

#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#if __cplusplus >= 201703L
#define COMMON_NODISCARD [[nodiscard]]
#else
#define COMMON_NODISCARD
#endif

namespace common {

//    |  N  |               | 32-bytes
//    |             | 32-bytes
//    |             | 32-bytes
//    |             | 32-bytes
//    |             | 32-bytes
//    |             | 32-bytes

class Packet {
 public:




 private:
  std::string buffer;
};

class Protocol {
 public:
  using string_type = std::string;
  using key_type = string_type;
  using value_type = string_type;
  using header_type = std::unordered_map<key_type, value_type>;

  template<typename Key, typename Value>
  std::enable_if<
    std::conjunction_v<
      std::is_constructible<string_type, Key>,
      std::is_constructible<string_type, Value>
    >
  >
  emplace(Key&& key, Value&& value) {
    if (auto it = header().find(key); it != header().end()) {
      std::cerr
        << "Key " << it->first << " already exists!"
        << " Existing key will be overwritten\n";
    }
    header_.emplace(std::forward<Key>(key), std::forward<Value>(value));
  }

   void set_content(string_type data) {
     if (!content_.empty()) {
       std::cerr << "Content already exists! Existing content will be overwritten.\n";
     }

     content_ = std::move(data);
   }

  COMMON_NODISCARD header_type& header() { return header_; }
  COMMON_NODISCARD const header_type& header() const { return header_; }

  COMMON_NODISCARD std::vector<Packet> build() const {


    return {};
  }

 private:
  header_type header_;
  string_type content_;
};

} // namespace common

#endif // SERVER_NETWORK_PROTOCOL_H_
