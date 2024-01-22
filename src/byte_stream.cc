#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  // Your code here.
  if ( is_closed() || data.empty() ) {
    return;
  }

  const uint64_t data_length = data.length();
  const uint64_t available_length = available_capacity();
  const uint64_t length = data_length >= available_length ? available_length : data_length;

  if ( data_length >= available_length ) {
    buffer_.append( data.substr( 0, length ) );
  } else {
    buffer_.append( std::move( data ) );
  }
  bytes_pushed_ += length;
}

void Writer::close()
{
  // Your code here.
  closed_ = true;
}

void Writer::set_error()
{
  // Your code here.
  error_ = true;
}

bool Writer::is_closed() const
{
  // Your code here.
  return closed_;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - bytes_pushed_ + bytes_popped_;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_pushed_;
}

string_view Reader::peek() const
{
  // Your code here.
  return { buffer_ };
}

bool Reader::is_finished() const
{
  // Your code here.
  return closed_ && buffer_.empty();
}

bool Reader::has_error() const
{
  // Your code here.
  return error_;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  if ( buffer_.empty() ) {
    return;
  }

  uint64_t buffer_length = buffer_.length();
  uint64_t length = len >= buffer_length ? buffer_length : len;

  buffer_.erase( 0, length );
  bytes_popped_ += length;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return buffer_.length();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_popped_;
}
