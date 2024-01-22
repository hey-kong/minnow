#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  // Return if SYN is not received and the incoming message does not contain it.
  if ( !syn_received_ && !message.SYN ) {
    return;
  }

  // Calculate the current checkpoint in the inbound stream.
  uint64_t checkpoint = syn_received_ + inbound_stream.bytes_pushed();

  // Initialize TCPReceiver's seqno if SYN is contained in the message.
  if ( !syn_received_ && message.SYN ) {
    syn_received_ = true;
    zero_point_ = message.seqno;
  }

  // Extract data from the message and insert into the reassembler.
  string data = message.payload;
  uint64_t first_index = message.seqno.unwrap( zero_point_.value(), checkpoint ) - ( !message.SYN );
  reassembler.insert( first_index, data, message.FIN, inbound_stream );
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  TCPReceiverMessage message;
  message.ackno = gen_ackno( inbound_stream );
  message.window_size = gen_window_size( inbound_stream );
  return message;
}

optional<Wrap32> TCPReceiver::gen_ackno( const Writer& inbound_stream ) const
{
  if ( !zero_point_.has_value() ) {
    return nullopt;
  } else {
    // Calculate the acknowledgment number.
    uint64_t bytes_pushed = inbound_stream.bytes_pushed();
    bool is_closed = inbound_stream.is_closed();
    return Wrap32::wrap( syn_received_ + bytes_pushed + is_closed, zero_point_.value() );
  }
}

uint16_t TCPReceiver::gen_window_size( const Writer& inbound_stream ) const
{
  // Calculate the available capacity in the inbound stream.
  uint64_t available_capacity = inbound_stream.available_capacity();
  return available_capacity > 0xffff ? 0xffff : available_capacity;
}
