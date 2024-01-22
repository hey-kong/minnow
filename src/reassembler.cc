#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  // Your code here.
  const uint64_t available_capacity = output.available_capacity();
  const uint64_t first_unacceptable = first_unassembled_ + available_capacity;
  const uint64_t data_length = data.length();

  // Return if the data is already assembled or beyond the acceptable range.
  if ( ( first_index + data_length <= first_unassembled_ && data_length != 0 )
       || first_index > first_unacceptable ) {
    return;
  }

  // If data exceeds the acceptable range, crop it and update is_last_substring flag.
  if ( first_index + data_length > first_unacceptable ) {
    data = data.substr( 0, first_unacceptable - first_index );
    is_last_substring = false;
  }

  // Insert or update data in the map.
  auto it = unassembled_.find( first_index );
  if ( it == unassembled_.end() ) {
    unassembled_[first_index] = data;
  } else {
    if ( data_length > it->second.length() ) {
      unassembled_[first_index] = data;
    }
  }

  // Iterate over the map and push assembled data to output.
  for ( it = unassembled_.begin(); it != unassembled_.end(); ) {
    // Break if a hole is found in the data.
    if ( it->first > first_unassembled_ ) {
      break;
    }

    // Remove fully assembled data.
    if ( it->first + it->second.length() <= first_unassembled_ ) {
      auto tmp = it;
      it++;
      unassembled_.erase( tmp );
    } else {
      // Push string to output and update the index.
      output.push( it->second.substr( first_unassembled_ - it->first ) );
      first_unassembled_ = it->first + it->second.length();
      auto tmp = it;
      it++;
      unassembled_.erase( tmp );
    }
  }

  // Set finish flag if the last substring is received.
  if ( is_last_substring ) {
    finish_received_ = true;
  }

  // Close the output if finished and no data is pending.
  if ( finish_received_ && bytes_pending() == 0 ) {
    output.close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  uint64_t bytes_count = 0;
  uint64_t largest_index = first_unassembled_;

  // Iterate over the map to count the bytes.
  for ( const auto& pair : unassembled_ ) {
    uint64_t start = pair.first;
    uint64_t end = start + pair.second.length();

    // Add bytes that are not yet assembled.
    if ( start <= largest_index ) {
      if ( end > largest_index ) {
        bytes_count += end - largest_index;
        largest_index = end;
      }
    } else {
      bytes_count += pair.second.length();
      largest_index = end;
    }
  }

  return bytes_count;
}
