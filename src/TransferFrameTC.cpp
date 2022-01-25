#include <TransferFrameTC.hpp>

TransferFrameTC::TransferFrameTC(uint8_t * transfer_frame, uint16_t transfer_frame_length, TransferFrameType t): TransferFrame(t, transfer_frame_length, transfer_frame), hdr(transfer_frame){
    // Segment header may be unavailable in virtual channels, here we treat it as if it's there sinc it's existence is
    // dependent on the channel. We deal with it's existence afterwards
    segHdr = transfer_frame[5];
    gvcid = transfer_frame[2] >> 2;

    // MAP IDs are relevant in case the transfer frame primary header is present. If it's not, this will be determined
    // upon processing the packet in the relevant channel since since this value will be essentially junk
    mapid = mapId() & 0x63;

    // todo: This is supposed to be a user-set number that will help with identification of a packet upon generating an
    // alert. However, I'm still unsure on the specifics so let's just leave this blank for now
    sduid = 0;
    serviceType = ServiceType::TYPE_A;
    transferFrameSeqNumber = transfer_frame[4];
    ack = false;
    toBeRetransmitted = false; // N/A
}