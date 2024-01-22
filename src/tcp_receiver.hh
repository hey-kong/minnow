#pragma once

#include "reassembler.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

class TCPReceiver
{
private:
  bool syn_received_ { false };
  std::optional<Wrap32> zero_point_ { std::nullopt };

  // Generate ackno for TCPReceiver message.
  std::optional<Wrap32> gen_ackno( const Writer& ) const;
  // Generate window_size for TCPReceiver message.
  uint16_t gen_window_size( const Writer& ) const;

public:
  /*
   * The TCPReceiver receives TCPSenderMessages, inserting their payload into the Reassembler
   * at the correct stream index.
   */
  void receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream );

  /* The TCPReceiver sends TCPReceiverMessages back to the TCPSender. */
  TCPReceiverMessage send( const Writer& inbound_stream ) const;
};
