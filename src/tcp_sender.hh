#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include <exception>
#include <functional>
#include <iostream>

class TCPSender
{
private:
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  bool syn_ { false };
  bool fin_ { false };

  // The number of bytes sent by the sender.
  uint64_t abs_seqno_ { 0 };
  // The number of bytes acked by the sender.
  uint64_t abs_ackno_ { 0 };
  // Receiver's window size.
  uint16_t window_size_ { 1 };
  // Segments that are not acked.
  std::deque<TCPSenderMessage> outstanding_segments_ {};
  // Segments that need to be sent.
  std::deque<TCPSenderMessage> queued_segments_ {};

  bool active_ { false };
  size_t timestamp_ { 0 };
  uint64_t consecutive_retransmissions_ { 0 };
  uint64_t cur_RTO_ { initial_RTO_ms_ };

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};
