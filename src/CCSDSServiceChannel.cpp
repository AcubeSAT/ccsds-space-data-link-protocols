#include <CCSDSServiceChannel.hpp>
#include <TransferFrame.hpp>
#include <etl/iterator.h>

void ServiceChannel::store(uint8_t* packet, uint16_t packet_length, uint8_t gvcid, uint8_t mapid, uint16_t sduid,
                           bool service_type){

    uint8_t vid = gvcid & 0x3F;
    MAPChannel *map_channel = &(masterChannel.virtChannels.at(vid).mapChannels.at(mapid));

    if (masterChannel.virtChannels.at(vid).mapChannels.at(mapid).packetList.full()){
       // Send indication that buffer is full
       return;
    }

    Packet packet_s = Packet(packet, packet_length, 0, gvcid, mapid, sduid, service_type);
    masterChannel.virtChannels.at(vid).mapChannels.at(mapid).packetList.push(packet_s);

}

void ServiceChannel::mapp_request(uint8_t vid, uint8_t mapid){
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    MAPChannel *map_channel = &(virt_channel->mapChannels.at(mapid));

    if (map_channel->packetList.empty()){
        // There's no packets to process
        return;
    }

    if (virt_channel->packetList.size()){
        // Log that there's no space for any packets to be stored in the virtual channel buffer
        return;
    }
    Packet packet = map_channel->packetList.front();

    const uint16_t max_frame_length = virt_channel->maxFrameLength;
    bool segmentation_enabled = virt_channel->segmentHeaderPresent;
    bool blocking_enabled = virt_channel->blocking;

    const uint16_t max_packet_length = max_frame_length - (TC_PRIMARY_HEADER_SIZE + segmentation_enabled*1U);

    if (packet.packetLength > max_packet_length){
        if (segmentation_enabled){
            // Check if there is enough space in the buffer of the virtual channel to store all the segments
            uint8_t tf_n = (packet.packetLength / max_packet_length) +  (packet.packetLength % max_packet_length != 0);

            if (virt_channel->packetList.capacity() >= tf_n){
                // Break up packet
                map_channel->packetList.pop();

                // First portion
                uint16_t seg_header = mapid && 0x40;

                Packet t_packet = Packet(packet.packet, max_packet_length, seg_header, packet.gvcid, packet.mapid, packet.sduid,
                        packet.serviceType);
                virt_channel->store(t_packet);

                // Middle portion
                t_packet.segHdr = 0x00;
                for (uint8_t i = 1; i < (tf_n - 1); i++){
                    t_packet.packet = &packet.packet[i*max_packet_length];
                    virt_channel->store(t_packet);
                }

                // Last portion
                t_packet.segHdr = 0x80;
                t_packet.packet = &packet.packet[(tf_n - 1) * max_packet_length];
                t_packet.packetLength = packet.packetLength % max_packet_length;
                virt_channel->store(t_packet);
            }
        } else{
            // Raise error that packet exceeds maximum size
            return;
        }
    } else{
        // We've already checked whether there is enough space in the buffer so we can simply remove the packet from
        // the buffer.
        map_channel->packetList.pop();

        if (blocking_enabled){
            // See if we can block it with other packets
            // @todo There are a few things I'm unsure about regarding blocking:
            // - You need to block neighbouring packets because otherwise there's a significant increase in complexity
            // when searching for the most space-efficient combination of packets (though it is still manageable for
            // fairly small buffer size) and you'd also need a linked list instead of a double queue
            // - Neighboring packets also need to share a primary header
            // - You'll need to allocate a new chunk of memory (unless you go with data structures that don't require
            // contiguous memory but I'm also against that)

            // for now just send packet as-is
            virt_channel->store(packet);
        } else{
            if (segmentation_enabled){
                    packet.segHdr = (0xc0) || (mapid && 0x3F);
                }
            virt_channel->store(packet);
        }
    }
}

void ServiceChannel::vc_generation_request(uint8_t vid){
    VirtualChannel virt_channel = std::move(masterChannel.virtChannels.at(vid));
    if (virt_channel.packetList.empty()){
        // There's no packets to process
           return;
    }

    if (masterChannel.framesList.empty()){
        // Log that there's no space for any packets to be stored in the virtual channel buffer
        return;
    }
    // Perform COP and stuff
}