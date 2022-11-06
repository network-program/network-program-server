//
// Created by YongGyu Lee on 2022/10/14.
//

#include "server/network/protocol.h"

#include <iostream>
#include <exception>

#define TEST_FAIL               \
do {                            \
  std::cerr                     \
      << "Test failed at "      \
      << __FILE__ << ", line "  \
      << __LINE__ << '\n';      \
  std::terminate();             \
} while(false)


class MockPacket
 : public network::BinaryPacket {
 public:
  enum {
    packet_size_byte    = sizeof(uint32_t),
    packet_size_offset  = 0,
    sequence_byte       = sizeof(uint32_t),
    current_sequence_byte = packet_size_offset + packet_size_byte,
    total_sequence_offset = current_sequence_byte + sequence_byte,
    packet_header_byte    = total_sequence_offset + sequence_byte,
  };

  explicit MockPacket(size_t buffer_size) : BinaryPacket(buffer_size) {}
  NETWORK_NODISCARD uint32_t current_sequence() const { return get<uint32_t>(current_sequence_byte); }
  NETWORK_NODISCARD uint32_t total_sequence() const { return get<uint32_t>(total_sequence_offset); }
  NETWORK_NODISCARD std::string_view content() const { return data() + packet_header_byte; }

 private:
};

class MockPacketGenerator
 : public network::BasicPacketGenerator<MockPacket> {
 public:
  using base = BasicPacketGenerator<MockPacket>;
  using packet = MockPacket;
  using string_type = std::string;

  MockPacketGenerator(uint32_t packet_size, string_type data)
    : base(data.empty() ? 0 : ((data.size() - 1) / (packet_size - packet::packet_header_byte)) + 1,
           packet_size,
           data),
      max_packet_size_(packet_size)
  {
    NETWORK_ASSERT(max_packet_size_ > packet::packet_header_byte,
                   "Packet size must be greater than packet header size");
  }

  std::optional<packet> GenerateNext() override {
    if (write_offset_ >= base::data().size()) {
      return {};
    }

    const uint32_t packet_header_size = packet::packet_header_byte;
    const uint32_t packet_size = std::min(max_packet_size_, static_cast<uint32_t>(base::data().size()) + packet_header_size);
    const uint32_t packet_data_size = packet_size - packet_header_size;
    MockPacket packet(packet_size);

    packet << packet_size;
    packet << ++generated_packet_index_;
    packet << total_packet_num_;

#ifndef NDEBUG
    NETWORK_ASSERT(packet.used_size() == sizeof(uint32_t) * 3, "Invalid write index");
#endif
    packet << base::data().substr(write_offset_, packet_data_size);
    write_offset_ += packet_data_size;

    return packet;
  }

  NETWORK_NODISCARD auto total_packet_num() const { return total_packet_num_; }
  NETWORK_NODISCARD auto generated_packet_num() const { return generated_packet_index_; }

 private:
  typename string_type::size_type write_offset_ = 0;
  const uint32_t max_packet_size_;
};

int main() {
  using Protocol = network::BasicProtocol<1'000'000, MockPacketGenerator>;

  {
    Protocol protocol(":", ";");

    protocol.add_header("foo", "bar");
    protocol.add_header("name", "yonggyulee");
    protocol.add_header("ID", "17010370");
    protocol.set_content("This is content");

    auto packet_builder = protocol.build();

    while (auto packet = packet_builder.GenerateNext()) {
      std::cout
        << packet->size() << " bytes( "
        << packet->current_sequence() << " / " << packet->total_sequence() << " ) : "
        << packet->content() << '\n';

      if (packet->content() != "foo:bar;name:yonggyulee;ID:17010370;;This is content") {
        TEST_FAIL;
      }
    }
  }

  { // Parse test 1
    Protocol protocol("=>", "\r\n");

    protocol.parse("Header1=>Value1\r\nHeader2=>Value2\r\n\r\nHello This is a content");

    Protocol::header_type::const_iterator it;
    if (it = protocol.header().find("Header1"); it == protocol.header().end()) TEST_FAIL;
    if (it->second != "Value1") TEST_FAIL;
    if (it = protocol.header().find("Header2"); it == protocol.header().end()) TEST_FAIL;
    if (it->second != "Value2") TEST_FAIL;
    if (protocol.content() != "Hello This is a content") TEST_FAIL;
  }

  { // Parse test 2
    Protocol protocol(":", "\r\n");

    protocol.parse("H:V\r\nH2:V2\r\n\r\nHello This is a content\nWith a newline!");

    Protocol::header_type::const_iterator it;
    if (it = protocol.header().find("H"); it == protocol.header().end()) TEST_FAIL;
    if (it->second != "V") TEST_FAIL;
    if (it = protocol.header().find("H2"); it == protocol.header().end()) TEST_FAIL;
    if (it->second != "V2") TEST_FAIL;
    if (protocol.content() != "Hello This is a content\nWith a newline!") TEST_FAIL;
  }

  {
    network::BasicProtocol<20, MockPacketGenerator> small_packet_protocol(":", ";");
    small_packet_protocol.add_header("header", "value"); // 11 bytes + 2 bytes
    small_packet_protocol.set_content("Hello, world!");  // 1 byte + 13 bytes
    // Total 27 bytes, for 12 bytes for each header -> (12 + 8) * 3 + (12 + 3) packets

    auto builder = small_packet_protocol.build();
    if (builder.total_packet_num() != 4) TEST_FAIL;

    if (const auto p = builder.GenerateNext(); p->content() != "header:v") TEST_FAIL;
    if (const auto p = builder.GenerateNext(); p->content() != "alue;;He") TEST_FAIL;
    if (const auto p = builder.GenerateNext(); p->content() != "llo, wor") TEST_FAIL;
    if (const auto p = builder.GenerateNext(); p->content() != "ld!") TEST_FAIL;
    if (const auto p = builder.GenerateNext(); p.has_value()) TEST_FAIL;
  }

  return EXIT_SUCCESS;
}
