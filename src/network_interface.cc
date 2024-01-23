#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
  , mappings_()
  , ready_frames_()
  , unready_frames_()
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  /**
   * Find if the MAC corresponding to next_hop is known:
   *  1. If the MAC address is known, create an Ethernet frame, set it up, and send.
   *  2. If the MAC address is unknown, broadcast an ARP request to find the MAC address
   *     corresponding to this IP address, and store this <InternetDatagram, next_hop>.
   *     Once the MAC address for the IP is received, send the network datagram.
   */
  EthernetFrame frame;

  // Check if the MAC address for next_hop is known.
  if ( mappings_.contains( next_hop.ipv4_numeric() ) ) {
    auto [eAddress, time] = mappings_[next_hop.ipv4_numeric()];
    frame.header.type = EthernetHeader::TYPE_IPv4;
    frame.header.dst = eAddress;
    frame.header.src = ethernet_address_;
    frame.payload = serialize( dgram );
    ready_frames_.push_back( frame );
  } else {
    // Store the datagram and next_hop for later sending.
    unready_frames_.emplace_back( dgram, next_hop );
    // Send ARP request.
    if ( !arp_times_.contains( next_hop.ipv4_numeric() ) ) {
      arp_times_[next_hop.ipv4_numeric()] = timestamp_;
      // Create ARP Message.
      ARPMessage arpMessage;
      arpMessage.opcode = ARPMessage::OPCODE_REQUEST;
      arpMessage.sender_ip_address = ip_address_.ipv4_numeric();
      arpMessage.sender_ethernet_address = ethernet_address_;
      arpMessage.target_ip_address = next_hop.ipv4_numeric();
      arpMessage.target_ethernet_address = {};
      // Create Ethernet Message.
      frame.header.src = ethernet_address_;
      frame.header.dst = ETHERNET_BROADCAST;
      frame.header.type = EthernetHeader::TYPE_ARP;
      frame.payload = serialize( arpMessage );
      ready_frames_.push_back( frame );
    }
  }
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Check for IPv4 type frame and process it.
  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram dgram;
    // Return parsed datagram if destination address matches.
    if ( parse( dgram, frame.payload ) && frame.header.dst == ethernet_address_ ) {
      return dgram;
    }
  } else if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    // Process ARP frames.
    ARPMessage arpMessage;

    // Return if parsing fails.
    if ( !parse( arpMessage, frame.payload ) ) {
      return {};
    }

    // Update ARP table with sender information.
    mappings_[arpMessage.sender_ip_address] = { arpMessage.sender_ethernet_address, timestamp_ };

    // Erase ARP request timestamp if existing.
    arp_times_.erase( arpMessage.sender_ip_address );

    // Process unready frames now that we have new ARP information.
    auto it = unready_frames_.begin();
    while ( it != unready_frames_.end() ) {
      auto [dgram, addr] = *it;
      if ( mappings_.contains( addr.ipv4_numeric() ) ) {
        send_datagram( dgram, addr );
        it = unready_frames_.erase( it );
      } else {
        ++it;
      }
    }

    // Send ARP reply if the frame is an ARP request for this interface's IP.
    if ( arpMessage.opcode == ARPMessage::OPCODE_REQUEST
         && arpMessage.target_ip_address == ip_address_.ipv4_numeric() ) {
      // Create ARP Message.
      ARPMessage message;
      message.opcode = ARPMessage::OPCODE_REPLY;
      message.sender_ip_address = ip_address_.ipv4_numeric();
      message.sender_ethernet_address = ethernet_address_;
      message.target_ip_address = arpMessage.sender_ip_address;
      message.target_ethernet_address = arpMessage.sender_ethernet_address;
      // Create Ethernet Message.
      EthernetFrame reply_frame;
      reply_frame.header.type = EthernetHeader::TYPE_ARP;
      reply_frame.header.src = ethernet_address_;
      reply_frame.header.dst = arpMessage.sender_ethernet_address;
      reply_frame.payload = serialize( message );
      ready_frames_.push_back( reply_frame );
    }
  }
  return {};
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  timestamp_ += ms_since_last_tick;

  // Remove expired ARP times.
  for ( auto it = arp_times_.begin(); it != arp_times_.end(); ) {
    if ( timestamp_ - it->second >= 5000 ) {
      it = arp_times_.erase( it );
    } else {
      ++it;
    }
  }

  // Delete expired mappings.
  for ( auto it = mappings_.begin(); it != mappings_.end(); ) {
    if ( timestamp_ - it->second.second >= 30000 ) {
      it = mappings_.erase( it );
    } else {
      ++it;
    }
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if ( ready_frames_.empty() ) {
    return {};
  }

  EthernetFrame frame = move( ready_frames_.front() );
  ready_frames_.pop_front();
  return frame;
}
