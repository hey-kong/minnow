#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), initial_RTO_ms_( initial_RTO_ms )
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return abs_seqno_ - abs_ackno_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return consecutive_retransmissions_;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  // Return empty if there are no segments queued for sending.
  if ( queued_segments_.empty() ) {
    return {};
  }

  TCPSenderMessage message = queued_segments_.front();
  queued_segments_.pop_front();

  // Add the message to the segments awaiting acknowledgment.
  outstanding_segments_.push_back( message );

  // Activate the sender if it was not active.
  active_ = true;

  return message;
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  // Return immediately if FIN flag is already set.
  if ( fin_ ) {
    return;
  }

  const uint64_t cur_window_size = window_size_ == 0 ? 1 : window_size_;

  while ( cur_window_size > sequence_numbers_in_flight() ) {
    TCPSenderMessage message;

    // Set SYN flag for the first message.
    if ( !syn_ ) {
      syn_ = true;
      message.SYN = true;
    }

    message.seqno = isn_ + abs_seqno_;
    size_t length_to_read = min( { TCPConfig::MAX_PAYLOAD_SIZE,
                                   static_cast<size_t>( outbound_stream.bytes_buffered() ),
                                   cur_window_size - sequence_numbers_in_flight() } );

    read( outbound_stream, length_to_read, message.payload );

    // Set FIN flag if stream is finished and there is space in the window.
    if ( !fin_ && outbound_stream.is_finished()
         && length_to_read + message.SYN + sequence_numbers_in_flight() < cur_window_size ) {
      fin_ = true;
      message.FIN = true;
    }

    // Break if there is no data to send.
    if ( message.sequence_length() == 0 ) {
      break;
    }

    queued_segments_.push_back( message );
    abs_seqno_ += message.sequence_length();
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.
  TCPSenderMessage message {};
  message.seqno = isn_ + abs_seqno_;
  return message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  if ( msg.ackno.has_value() ) {
    auto received_seqno = msg.ackno->unwrap( isn_, abs_seqno_ );

    // Ignore invalid acknowledgements.
    if ( received_seqno > abs_seqno_ || received_seqno < abs_ackno_ ) {
      return;
    }
    abs_ackno_ = received_seqno;
  }

  window_size_ = msg.window_size;

  // Remove acknowledged segments and reset the timer if necessary
  while ( !outstanding_segments_.empty() ) {
    auto& segment = outstanding_segments_.front();
    auto last_seqno = ( segment.seqno + segment.sequence_length() ).unwrap( isn_, abs_seqno_ );

    if ( last_seqno <= abs_ackno_ ) {
      outstanding_segments_.pop_front();
      // If a valid ackno is received by the sender, the timer needs to be reset
      if ( window_size_ != 0 ) {
        timestamp_ = 0;
        cur_RTO_ = initial_RTO_ms_;
        consecutive_retransmissions_ = 0;
      }
    } else {
      break;
    }
  }

  // If there are no more segments sent but not acknowledged, then stop the timer
  active_ = !outstanding_segments_.empty();
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  if ( !active_ ) {
    return;
  }

  timestamp_ += ms_since_last_tick;

  if ( timestamp_ >= cur_RTO_ && !outstanding_segments_.empty() ) {
    // Retransmit the oldest segment when the timer expires
    queued_segments_.push_back( outstanding_segments_.front() );
    if ( window_size_ > 0 ) {
      consecutive_retransmissions_++;
      cur_RTO_ = pow( 2, consecutive_retransmissions_ ) * initial_RTO_ms_;
    }
    timestamp_ = 0;
  }
}
