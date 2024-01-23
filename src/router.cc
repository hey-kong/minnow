#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  routing_table_.push_back( RouterEntry( route_prefix, prefix_length, next_hop, interface_num ) );
}

void Router::forward_datagram( InternetDatagram& dgram )
{
  // Drop the datagram if TTL is 1 or less.
  if ( dgram.header.ttl <= 1 ) {
    return;
  }

  // Longest prefix match to find the route.
  uint32_t destination = dgram.header.dst;
  int match_idx = -1;
  int max_matched_len = -1;
  for ( size_t i = 0; i < routing_table_.size(); i++ ) {
    auto mask = routing_table_[i].prefix_length == 0
                  ? 0
                  : numeric_limits<int>::min() >> ( routing_table_[i].prefix_length - 1 );
    if ( ( destination & mask ) == routing_table_[i].route_prefix
         && max_matched_len < routing_table_[i].prefix_length ) {
      match_idx = i;
      max_matched_len = routing_table_[i].prefix_length;
    }
  }

  // Drop the datagram if no route is found.
  if ( match_idx == -1 ) {
    return;
  }

  // Process and send the datagram.
  dgram.header.ttl -= 1;
  dgram.header.compute_checksum();
  auto next_hop = routing_table_[match_idx].next_hop;
  auto interface_num = routing_table_[match_idx].interface_num;
  if ( next_hop.has_value() ) {
    interfaces_[interface_num].send_datagram( dgram, next_hop.value() );
  } else {
    interfaces_[interface_num].send_datagram( dgram, Address::from_ipv4_numeric( destination ) );
  }
}

void Router::route()
{
  // Route every incoming datagram on each interface.
  for ( auto& interface : interfaces_ ) {
    optional<InternetDatagram> maybe_dgram;
    while ( ( maybe_dgram = interface.maybe_receive() ) != nullopt ) {
      forward_datagram( maybe_dgram.value() );
    }
  }
}
