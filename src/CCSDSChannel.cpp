#include <CCSDSChannel.hpp>
#include <Alert.hpp>
#include "CCSDSLoggerImpl.h"
// Virtual Channel

VirtualChannelAlert VirtualChannel::storeVC(PacketTC *packet) {
    // Limit the amount of packets that can be stored at any given time
    if (txUnprocessedPacketListBufferTC.full()) {
        ccsdsLog(Tx, TypeVirtualChannelAlert, TX_WAIT_QUEUE_FULL);
        return VirtualChannelAlert::TX_WAIT_QUEUE_FULL;
    }
    txUnprocessedPacketListBufferTC.push_back(packet);
    ccsdsLog(Tx, TypeVirtualChannelAlert, NO_VC_ALERT);
    return VirtualChannelAlert::NO_VC_ALERT;
}

etl::queue<std::pair<uint8_t *, uint16_t>, PacketBufferTmSize> VirtualChannel::getPacketPtrBufferTm() {
    return packetPtrBufferTm;
}

etl::queue<uint8_t, PacketBufferTmSize> VirtualChannel::getPacketBufferTm() {
    return packetBufferTm;
}

void VirtualChannel::setPacketPtrBufferTm(etl::queue<std::pair<uint8_t *, uint16_t>, PacketBufferTmSize> &packetPtrBuffer) {
    this->packetPtrBufferTm = packetPtrBuffer;
}

void VirtualChannel::setPacketBufferTm(etl::queue<uint8_t, PacketBufferTmSize> &packetBuffer) {
    this->packetBufferTm = packetBuffer;
}

void VirtualChannel::storePacketInPacketBufferTm(uint8_t *packet, uint16_t packetLength) {
    packet = virtualChannelPool.allocatePacket(packet, packetLength);
    if (packet != nullptr) {
        std::pair<uint8_t *, uint16_t> packetPtr;
        packetPtr.first = packet;
        packetPtr.second = packetLength;
        packetPtrBufferTm.push(packetPtr);
        for (uint16_t i = 0; i < packetLength; i++) {
            packetBufferTm.push(packet[i]);
        }
    }
}

// Master Channel

// Technically not a packet, but it has identical information
// @todo consider another data structure

MasterChannelAlert MasterChannel::storeOut(PacketTC *packet) {
    if (txOutFramesBeforeAllFramesGenerationList.full()) {
        // Log that buffer is full
        ccsdsLog(Tx, TypeMasterChannelAlert, OUT_FRAMES_LIST_FULL);
        return MasterChannelAlert::OUT_FRAMES_LIST_FULL;
    }
    txOutFramesBeforeAllFramesGenerationList.push_back(packet);
    uint8_t vid = packet->globalVirtualChannelId();
    // virtChannels.at(0).fop.
    ccsdsLog(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
    return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::storeTransmittedOut(PacketTC *packet) {
    if (txToBeTransmittedFramesAfterAllFramesGenerationList.full()) {
        ccsdsLog(Tx, TypeMasterChannelAlert, TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
        return MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
    }
    txToBeTransmittedFramesAfterAllFramesGenerationList.push_back(packet);
    ccsdsLog(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
    return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::addVC(const uint8_t vcid, const bool segmentHeaderPresent,
                                        const uint16_t maxFrameLength, const uint8_t clcwRate, const bool blocking,
                                        const uint8_t repetitionTypeAFrame, const uint8_t repetitionCopCtrl,
                                        const uint8_t frameCountP,
                                        etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChan) {
    if (virtChannels.full()) {
        ccsdsLog(Tx, TypeMasterChannelAlert, MAX_AMOUNT_OF_VIRT_CHANNELS);
        return MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS;
    }

    virtChannels.emplace(vcid, VirtualChannel(*this, vcid, segmentHeaderPresent, maxFrameLength, clcwRate,
                                              blocking,
                                              repetitionTypeAFrame, repetitionCopCtrl, frameCountP, mapChan));
    return MasterChannelAlert::NO_MC_ALERT;
}

void MasterChannel::mergePacketsToTransferFrame(VirtualChannel *vc, uint16_t maxTransferFrameDataLength) {
    uint16_t transferFrameDataLength = 0;
    etl::queue<std::pair<uint8_t *, uint16_t>, PacketBufferTmSize> packetPtrBufferTm = vc->getPacketPtrBufferTm();
    etl::queue<uint8_t, PacketBufferTmSize> packetBufferTm = vc->getPacketBufferTm();
    uint16_t packetLength = packetPtrBufferTm.front().second;
    while (transferFrameDataLength + packetLength <= maxTransferFrameDataLength) {
        transferFrameDataLength += packetLength;
        for (uint16_t i = 0; i < packetLength; i++) {
            masterTransferFrameDataBufferTm.push(packetBufferTm.front());
            packetBufferTm.pop();
        }
        packetPtrBufferTm.pop();
        packetLength = packetPtrBufferTm.front().second;
    }
    //Add ones as padding
    for (uint16_t i = 0; i < maxTransferFrameDataLength - transferFrameDataLength; i++) {
        masterTransferFrameDataBufferTm.push(1);
    }
    masterTransferFramePtrBufferTm.push(&masterTransferFrameDataBufferTm.back() - maxTransferFrameDataLength);
    vc->setPacketPtrBufferTm(packetPtrBufferTm);
    vc->setPacketBufferTm(packetBufferTm);
}