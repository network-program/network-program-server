//
// Created by YongGyu Lee on 2022/10/14.
//

#include "server/network/protocol.h"

#include <iostream>

int main() {

  {
    network::Protocol protocol(":", ";");

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
        return EXIT_FAILURE;
      }
    }
  }

  {
    network::BasicProtocol<20> small_packet_protocol(":", ";");
    small_packet_protocol.add_header("header", "value"); // 11 bytes + 2 bytes
    small_packet_protocol.set_content("Hello, world!");  // 1 byte + 13 bytes
    // Total 27 bytes, for 12 bytes for each header -> (12 + 8) * 3 + (12 + 3) packets

    auto builder = small_packet_protocol.build();
    if (builder.total_packet_num() != 4) return EXIT_FAILURE;


    if (const auto p = builder.GenerateNext(); p->content() != "header:v") return EXIT_FAILURE;
    if (const auto p = builder.GenerateNext(); p->content() != "alue;;He") return EXIT_FAILURE;
    if (const auto p = builder.GenerateNext(); p->content() != "llo, wor") return EXIT_FAILURE;
    if (const auto p = builder.GenerateNext(); p->content() != "ld!") return EXIT_FAILURE;
    if (const auto p = builder.GenerateNext(); p.has_value()) return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
