#include <CCSDSServiceChannel.hpp>
#include <TransferFrameTM.hpp>
#include <etl/iterator.h>
#include "CLCW.hpp"

// TC TransferFrame - Sending End (TC Tx)

//     - Utility and Debugging
TransferFrameTC ServiceChannel::frontUnprocessedFrameMcCopyTxTC() {
    return masterChannel.geFirstTxMasterCopyTcFrame();
}

TransferFrameTC ServiceChannel::getLastMasterCopyTcFrame() {
    return masterChannel.getLastTxMasterCopyTcFrame();
}

std::pair<ServiceChannelNotification, const TransferFrameTC*> ServiceChannel::frontUnprocessedFrameVcCopyTxTC(uint8_t vid) const {
    const etl::list<TransferFrameTC*, MaxReceivedUnprocessedTxTcInVirtBuffer>* vc =
            &(masterChannel.virtualChannels.at(vid).unprocessedFrameListBufferVcCopyTxTC);
    if (vc->empty()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
    }
    ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, vc->front());
}


std::pair<ServiceChannelNotification, const TransferFrameTC*> ServiceChannel::backUnprocessedFrameMcCopyTxTC() const {
    if (masterChannel.masterCopyTxTC.empty()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
    }
    return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, &(masterChannel.masterCopyTxTC.back()));
}

std::pair<ServiceChannelNotification, const TransferFrameTC*> ServiceChannel::frontFrameAfterAllFramesGenerationTxTC() const {
    if (masterChannel.toBeTransmittedFramesAfterAllFramesGenerationListTxTC.empty()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
    }
    ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT,
                     masterChannel.toBeTransmittedFramesAfterAllFramesGenerationListTxTC.front());
}

std::optional<TransferFrameTC> ServiceChannel::frontFrameBeforeAllFramesGenerationTxTC() {
    if (masterChannel.outFramesBeforeAllFramesGenerationListTxTC.empty()) {
        return {};
    }

    TransferFrameTC frame = *masterChannel.outFramesBeforeAllFramesGenerationListTxTC.front();
    return frame;
}

//     - MAP Packet Processing
ServiceChannelNotification ServiceChannel::segmentationTC(uint8_t numberOfTransferFrames, uint16_t packetLength,
                                                          uint16_t transferFrameDataFieldLength, uint8_t vid, uint8_t mapid,
                                                          ServiceType service_type) {
    VirtualChannel& virtualChannel = masterChannel.virtualChannels.at(vid);
    MAPChannel& mapChannel = virtualChannel.mapChannels.at(mapid);

    etl::queue<uint16_t, PacketBufferTcSize> *packetLengthBufferTcTx;
    etl::queue<uint8_t, PacketBufferTcSize> *packetBufferTcTx;

    if (service_type == ServiceType::TYPE_AD){
        packetLengthBufferTcTx = &mapChannel.packetLengthBufferTxTcTypeA;
        packetBufferTcTx = &mapChannel.packetBufferTxTcTypeA;
    }
    else {
        packetLengthBufferTcTx = &mapChannel.packetLengthBufferTxTcTypeB;
        packetBufferTcTx = &mapChannel.packetBufferTxTcTypeB;
    }

    uint16_t currenttransferFrameDataFieldLength = 0;
    uint8_t tcTrailerSize = 2*virtualChannel.frameErrorControlFieldPresent;

    static uint8_t tmpData[MaxTcTransferFrameSize] = {0};
    for (uint16_t i = 0; i < numberOfTransferFrames; i++) {
        currenttransferFrameDataFieldLength =
                packetLength > transferFrameDataFieldLength ? transferFrameDataFieldLength : packetLength;
        SegmentLengthID segmentLengthId = SegmentationMiddle;
        for (uint16_t j = 0; j < currenttransferFrameDataFieldLength; j++) {
            tmpData[j + TmPrimaryHeaderSize] = packetBufferTcTx->front();
            packetBufferTcTx->pop();
        }
        if (i == 0) {
            segmentLengthId = SegmentationStart;
        } else if (i == numberOfTransferFrames - 1) {
            segmentLengthId = SegmentaionEnd;
        }
        for (uint8_t j = 0; j < TmTrailerSize; j++) {
            if (currenttransferFrameDataFieldLength + TcPrimaryHeaderSize + i < MaxTcTransferFrameSize) {
                tmpData[currenttransferFrameDataFieldLength + TcPrimaryHeaderSize + i] = 0;
            }
        }
        uint8_t* transferFrameData = masterChannel.masterChannelPoolTC.allocatePacket(
                tmpData, currenttransferFrameDataFieldLength + TcPrimaryHeaderSize + tcTrailerSize);
        if (transferFrameData == nullptr) {
            return MEMORY_POOL_FULL;
        }
        TransferFrameTC transferFrameTc =
                TransferFrameTC(transferFrameData, currenttransferFrameDataFieldLength + TcPrimaryHeaderSize + tcTrailerSize,
                                vid, service_type, true);
        masterChannel.masterCopyTxTC.push_back(transferFrameTc);
        virtualChannel.unprocessedFrameListBufferVcCopyTxTC.push_back(&(masterChannel.masterCopyTxTC.back()));
        packetLength -= transferFrameDataFieldLength;
    }
    packetLengthBufferTcTx->pop();
    return NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::blockingTC(uint16_t transferFrameDataFieldLength, uint16_t packetLength,
                                                      uint8_t vid, uint8_t mapid, ServiceType serviceType){
    uint16_t currentTransferFrameDataFieldLength = 0;
    static uint8_t tmpData[MaxTcTransferFrameSize] = {0};

    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
        return ServiceChannelNotification::INVALID_VC_ID;
    }

    VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

    if (vchan.segmentHeaderPresent && (vchan.mapChannels.find(mapid) == vchan.mapChannels.end())) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_MAP_ID);
        return ServiceChannelNotification::INVALID_MAP_ID;
    }

    MAPChannel& mapChannel = vchan.mapChannels.at(mapid);

    etl::queue<uint16_t, PacketBufferTcSize> *packetLengthBufferTcTx;
    etl::queue<uint8_t, PacketBufferTcSize> *packetBufferTcTx;

    if (serviceType == ServiceType::TYPE_AD){
        packetLengthBufferTcTx = &mapChannel.packetLengthBufferTxTcTypeA;
        packetBufferTcTx = &mapChannel.packetBufferTxTcTypeA;
    }
    else {
        packetLengthBufferTcTx = &mapChannel.packetLengthBufferTxTcTypeB;
        packetBufferTcTx = &mapChannel.packetBufferTxTcTypeB;
    }

    while (currentTransferFrameDataFieldLength + packetLength <= transferFrameDataFieldLength &&
           !packetLengthBufferTcTx->empty()) {
        for (uint16_t i = currentTransferFrameDataFieldLength; i < currentTransferFrameDataFieldLength + packetLength; i++) {
            tmpData[i + TcPrimaryHeaderSize] = packetBufferTcTx->front();
            packetBufferTcTx->pop();
        }
        currentTransferFrameDataFieldLength += packetLength;
        packetLengthBufferTcTx->pop();
        packetLength = packetLengthBufferTcTx->front();
        // If blocking is disabled, stop the operation on the first transfer frame
        if (!vchan.blocking) {
            break;
        }
    }

    uint8_t tcTrailerSize = 2 * vchan.frameErrorControlFieldPresent;

    uint8_t* transferFrameData = masterChannel.masterChannelPoolTC.allocatePacket(
            tmpData, currentTransferFrameDataFieldLength + TcPrimaryHeaderSize + tcTrailerSize);
    if (transferFrameData == nullptr) {
        return MEMORY_POOL_FULL;
    }
    TransferFrameTC transferFrameTc =
            TransferFrameTC(transferFrameData, currentTransferFrameDataFieldLength + TcPrimaryHeaderSize + tcTrailerSize,
                            vid, serviceType, true);
    masterChannel.masterCopyTxTC.push_back(transferFrameTc);
    vchan.unprocessedFrameListBufferVcCopyTxTC.push_back(&(masterChannel.masterCopyTxTC.back()));
    return NO_SERVICE_EVENT;
}


ServiceChannelNotification ServiceChannel::storePacketTxTC(uint8_t *packet, uint16_t packetLength, uint8_t vid, uint8_t mapid,
                                                           ServiceType service_type) {
    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
        return ServiceChannelNotification::INVALID_VC_ID;
    }

    VirtualChannel *vchan = &(masterChannel.virtualChannels.at(vid));

    if (vchan->mapChannels.find(mapid) == vchan->mapChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_MAP_ID);
        return ServiceChannelNotification::INVALID_MAP_ID;
    }

    MAPChannel* mapChannel = &(vchan->mapChannels.at(mapid));

    etl::queue<uint16_t, PacketBufferTmSize> *packetLengthBufferTcTx;
    etl::queue<uint8_t, PacketBufferTmSize> *packetBufferTcTx;

    if (service_type == ServiceType::TYPE_AD){
        packetLengthBufferTcTx = &mapChannel->packetLengthBufferTxTcTypeA;
        packetBufferTcTx = &mapChannel->packetBufferTxTcTypeA;
    }
    else {
        packetLengthBufferTcTx = &mapChannel->packetLengthBufferTxTcTypeB;
        packetBufferTcTx = &mapChannel->packetBufferTxTcTypeB;
    }

    if (packetLength <= packetBufferTcTx->available()) {
        packetLengthBufferTcTx->push(packetLength);
        for (uint16_t i = 0; i < packetLength; i++) {
            packetBufferTcTx->push(packet[i]);
        }
        return NO_SERVICE_EVENT;
    }
    return VC_MC_FRAME_BUFFER_FULL;
}

ServiceChannelNotification ServiceChannel::mappRequestTxTC(uint8_t vid, uint8_t mapid, uint8_t transferFrameDataLength, ServiceType serviceType) {
    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
        return ServiceChannelNotification::INVALID_VC_ID;
    }

    VirtualChannel& virtualChannel = masterChannel.virtualChannels.at(vid);

    if (virtualChannel.mapChannels.find(mapid) == virtualChannel.mapChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_MAP_ID);
        return ServiceChannelNotification::INVALID_MAP_ID;
    }

    MAPChannel& mapChannel = virtualChannel.mapChannels.at(mapid);

    etl::queue<uint16_t, PacketBufferTmSize> *packetLengthBufferTcTx;
    etl::queue<uint8_t, PacketBufferTmSize> *packetBufferTcTx;

    if (serviceType == ServiceType::TYPE_AD){
        packetLengthBufferTcTx = &mapChannel.packetLengthBufferTxTcTypeA;
        packetBufferTcTx = &mapChannel.packetBufferTxTcTypeA;
    }
    else {
        packetLengthBufferTcTx = &mapChannel.packetLengthBufferTxTcTypeB;
        packetBufferTcTx = &mapChannel.packetBufferTxTcTypeB;
    }

    if (virtualChannel.waitQueueTxTC.full()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, VC_MC_FRAME_BUFFER_FULL);

        return ServiceChannelNotification::VC_MC_FRAME_BUFFER_FULL;
    }

    static uint8_t tmpData[MaxTcTransferFrameSize] = {0, 0, 0, 0, 0};
    TransferFrameTC* frame = mapChannel.unprocessedFrameListBufferTC.front();

    uint16_t packetLength = packetLengthBufferTcTx->front();
    uint16_t numberOfTransferFrames =
            packetLength / transferFrameDataLength + (packetLength % transferFrameDataLength != 0);

    const uint16_t maxFrameLength = virtualChannel.maxFrameLengthTC;
    bool segmentationEnabled = virtualChannel.segmentHeaderPresent;

    const uint16_t maxPacketLength = maxFrameLength - (TcPrimaryHeaderSize + segmentationEnabled * 1U);

    if (numberOfTransferFrames >= 1) {
        return segmentationTC(numberOfTransferFrames, maxPacketLength, transferFrameDataLength, vid, mapid, serviceType);
    }

    blockingTC(transferFrameDataLength, packetLength, vid, mapid, serviceType);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

//     - Virtual Channel Generation
ServiceChannelNotification ServiceChannel::vcGenerationRequestTxTC(uint8_t vid) {
    VirtualChannel& virt_channel = masterChannel.virtualChannels.at(vid);
    if (virt_channel.unprocessedFrameListBufferVcCopyTxTC.empty()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
    }

    if (masterChannel.outFramesBeforeAllFramesGenerationListTxTC.full()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, TX_MC_FRAME_BUFFER_FULL);
        return ServiceChannelNotification::TX_MC_FRAME_BUFFER_FULL;
    }

    TransferFrameTC& frame = *virt_channel.unprocessedFrameListBufferVcCopyTxTC.front();
    COPDirectiveResponse err = COPDirectiveResponse::ACCEPT;

    err = virt_channel.fop.transferFdu();

    MasterChannelAlert mc = virt_channel.master_channel().storeOut(&frame);
    if (mc != MasterChannelAlert::NO_MC_ALERT) {
        ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, REJECT);
        return ServiceChannelNotification::FOP_REQUEST_REJECTED;
    }

    if (err == COPDirectiveResponse::REJECT) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, FOP_REQUEST_REJECTED);
        return ServiceChannelNotification::FOP_REQUEST_REJECTED;
    }
    virt_channel.unprocessedFrameListBufferVcCopyTxTC.pop_front();
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}


//         -- FOP Directives
ServiceChannelNotification ServiceChannel::frameTransmission(uint8_t* frameTarget) {
    if (masterChannel.toBeTransmittedFramesAfterAllFramesGenerationListTxTC.empty()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, TX_TO_BE_TRANSMITTED_FRAMES_LIST_EMPTY);
        return ServiceChannelNotification::TX_TO_BE_TRANSMITTED_FRAMES_LIST_EMPTY;
    }

    TransferFrameTC* frame = masterChannel.toBeTransmittedFramesAfterAllFramesGenerationListTxTC.front();
    frame->setRepetitions(frame->repetitions() - 1);
    frame->setToTransmitted();

    if (frame->repetitions() == 0) {
        masterChannel.toBeTransmittedFramesAfterAllFramesGenerationListTxTC.pop_front();
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    }
    memcpy(frameTarget, frame, frame->getFrameLength());
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::transmitAdFrame(uint8_t vid) {
    VirtualChannel* virt_channel = &(masterChannel.virtualChannels.at(vid));
    FOPNotification req;
    req = virt_channel->fop.transmitAdFrame();
    if (req == FOPNotification::NO_FOP_EVENT) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
        return ServiceChannelNotification::NO_SERVICE_EVENT;

    } else {
        // TODO
    }
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

// TODO: Probably not needed. Refactor sentQueueTC
ServiceChannelNotification ServiceChannel::pushSentQueue(uint8_t vid) {
    VirtualChannel* virt_channel = &(masterChannel.virtualChannels.at(vid));
    COPDirectiveResponse req;
    req = virt_channel->fop.pushSentQueue();

    if (req == COPDirectiveResponse::ACCEPT) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
        return ServiceChannelNotification::NO_SERVICE_EVENT;
    }
    ccsdsLogNotice(Tx, TypeServiceChannelNotif, TX_FOP_REJECTED);
    return ServiceChannelNotification::TX_FOP_REJECTED;
}

void ServiceChannel::acknowledgeFrame(uint8_t vid, uint8_t frameSeqNumber) {
    VirtualChannel* virt_channel = &(masterChannel.virtualChannels.at(vid));
    virt_channel->fop.acknowledgeFrame(frameSeqNumber);
}

void ServiceChannel::clearAcknowledgedFrames(uint8_t vid) {
    VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
    virtualChannel->fop.removeAcknowledgedFrames();
}

void ServiceChannel::initiateAdNoClcw(uint8_t vid) {
    VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
    FDURequestType req;
    req = virtualChannel->fop.initiateAdNoClcw();
}

void ServiceChannel::initiateAdClcw(uint8_t vid) {
    VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
    FDURequestType req;
    req = virtualChannel->fop.initiateAdClcw();
}

void ServiceChannel::initiateAdUnlock(uint8_t vid) {
    VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
    FDURequestType req;
    req = virtualChannel->fop.initiateAdUnlock();
}

void ServiceChannel::initiateAdVr(uint8_t vid, uint8_t vr) {
    VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
    FDURequestType req;
    req = virtualChannel->fop.initiateAdVr(vr);
}

void ServiceChannel::terminateAdService(uint8_t vid) {
    VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
    FDURequestType req;
    req = virtualChannel->fop.terminateAdService();
}

void ServiceChannel::resumeAdService(uint8_t vid) {
    VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
    FDURequestType req;
    req = virtualChannel->fop.resumeAdService();
}

void ServiceChannel::setVs(uint8_t vid, uint8_t vs) {
    VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
    virtualChannel->fop.setVs(vs);
}

void ServiceChannel::setFopWidth(uint8_t vid, uint8_t width) {
    VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
    virtualChannel->fop.setFopWidth(width);
}

void ServiceChannel::setT1Initial(uint8_t vid, uint16_t t1Init) {
    VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
    virtualChannel->fop.setT1Initial(t1Init);
}

void ServiceChannel::setTransmissionLimit(uint8_t vid, uint8_t vr) {
    VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
    virtualChannel->fop.setTransmissionLimit(vr);
}

void ServiceChannel::setTimeoutType(uint8_t vid, bool vr) {
    VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
    virtualChannel->fop.setTimeoutType(vr);
}
// todo: this may not be needed since it doesn't affect lower procedures and doesn't change the state in any way
void ServiceChannel::invalidDirective(uint8_t vid) {
    VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
    virtualChannel->fop.invalidDirective();
}
CLCW ServiceChannel::getClcwInBuffer() {
    CLCW clcw = CLCW(clcwTransferFrameBuffer.front().getOperationalControlField().value());
    return clcw;
}

uint8_t* ServiceChannel::getClcwTransferFrameDataBuffer() {
    return clcwTransferFrameDataBuffer;
}

FOPState ServiceChannel::fopState(uint8_t vid) const {
    return masterChannel.virtualChannels.at(vid).fop.state;
}

uint16_t ServiceChannel::t1Timer(uint8_t vid) const {
    return masterChannel.virtualChannels.at(vid).fop.tiInitial;
}

uint8_t ServiceChannel::fopSlidingWindowWidth(uint8_t vid) const {
    return masterChannel.virtualChannels.at(vid).fop.fopSlidingWindow;
}

bool ServiceChannel::timeoutType(uint8_t vid) const {
    return masterChannel.virtualChannels.at(vid).fop.timeoutType;
}

uint8_t ServiceChannel::transmitterFrameSeqNumber(uint8_t vid) const {
    return masterChannel.virtualChannels.at(vid).fop.transmitterFrameSeqNumber;
}

uint8_t ServiceChannel::expectedFrameSeqNumber(uint8_t vid) const {
    return masterChannel.virtualChannels.at(vid).fop.expectedAcknowledgementSeqNumber;
}

//     - All frames generation
ServiceChannelNotification ServiceChannel::allFramesGenerationRequestTxTC() {
    if (masterChannel.outFramesBeforeAllFramesGenerationListTxTC.empty()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
    }

    if (masterChannel.toBeTransmittedFramesAfterAllFramesGenerationListTxTC.full()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, TX_TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
        return ServiceChannelNotification::TX_TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
    }

    TransferFrameTC* frame = masterChannel.outFramesBeforeAllFramesGenerationListTxTC.front();
    masterChannel.outFramesBeforeAllFramesGenerationListTxTC.pop_front();

    uint8_t vid = frame->virtualChannelId();
    VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

    if (vchan.frameErrorControlFieldPresent) {
        frame->appendCRC();
    }

    masterChannel.storeTransmittedOut(frame);
    ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

// TC TransferFrame - Receiving End (TC Rx)

// Utility and debugging
std::pair<ServiceChannelNotification, const TransferFrameTC*> ServiceChannel::txOutFrameTC(uint8_t vid,
                                                                                           uint8_t mapid) const {
    const etl::list<TransferFrameTC*, MaxReceivedTcInMapChannel>* mc =
            &(masterChannel.virtualChannels.at(vid).mapChannels.at(mapid).unprocessedFrameListBufferTC);
    if (mc->empty()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
    }

    return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, mc->front());
}

//     - All Frames Reception
ServiceChannelNotification ServiceChannel::storeFrameRxTC(uint8_t* frameData, uint16_t frameLength) {
    if (masterChannel.masterCopyRxTC.full()) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_IN_MC_FULL);
        return ServiceChannelNotification::RX_IN_MC_FULL;
    }

    if (masterChannel.inFramesBeforeAllFramesReceptionListRxTC.full()) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_IN_BUFFER_FULL);
        return ServiceChannelNotification::RX_IN_BUFFER_FULL;
    }

    TransferFrameTC transferFrameTc = TransferFrameTC(frameData, frameLength);

    if (transferFrameTc.getFrameLength() != frameLength) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_INVALID_LENGTH);
        return ServiceChannelNotification::RX_INVALID_LENGTH;
    }

    uint8_t vid = transferFrameTc.virtualChannelId();
    uint8_t mapid = transferFrameTc.mapId();

    // Check if Virtual Channel Id does not exist in the relevant Virtual Channels map
    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        // If it doesn't, abort operation
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
        return ServiceChannelNotification::INVALID_VC_ID;
    }

    VirtualChannel* vchan = &(masterChannel.virtualChannels.at(vid));

    // If Segment Header present, check if MAP channel Id does not exist in the relevant MAP Channels map
    if (vchan->segmentHeaderPresent && (vchan->mapChannels.find(mapid) == vchan->mapChannels.end())) {
        // If it doesn't, abort the operation
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_MAP_ID);
        return ServiceChannelNotification::INVALID_MAP_ID;
    }

    masterChannel.masterCopyRxTC.push_back(transferFrameTc);
    TransferFrameTC* masterTransferFrameTc = &(masterChannel.masterCopyRxTC.back());

    masterChannel.inFramesBeforeAllFramesReceptionListRxTC.push_back(masterTransferFrameTc);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::allFramesReceptionRequestRxTC() {
    if (masterChannel.inFramesBeforeAllFramesReceptionListRxTC.empty()) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
        return ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS;
    }

    if (masterChannel.toBeTransmittedFramesAfterAllFramesReceptionListRxTC.full()) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_OUT_BUFFER_FULL);
        return ServiceChannelNotification::RX_OUT_BUFFER_FULL;
    }

    TransferFrameTC* frame = masterChannel.inFramesBeforeAllFramesReceptionListRxTC.front();
    VirtualChannel& virtualChannel = masterChannel.virtualChannels.at(frame->virtualChannelId());

    if (virtualChannel.waitQueueRxTC.full()) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, VC_RX_WAIT_QUEUE_FULL);
        return ServiceChannelNotification::VC_RX_WAIT_QUEUE_FULL;
    }

    // Frame Delimiting and Fill Removal supposedly aren't implemented here

    /*
     * Frame validation checks
     */

    // Check for valid TFVN
    if (frame->getTransferFrameVersionNumber() != 0) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_INVALID_TFVN);
        return ServiceChannelNotification::RX_INVALID_TFVN;
    }

    // Check for valid SCID
    if (frame->spacecraftId() != SpacecraftIdentifier) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_INVALID_SCID);
        return ServiceChannelNotification::RX_INVALID_SCID;
    }

    // TransferFrameTC length is checked upon storing the transferFrameData in the MC

    // If present in channel, check if CRC is valid
    bool eccFieldExists = virtualChannel.frameErrorControlFieldPresent;

    if (eccFieldExists) {
        uint16_t len = frame->frameLength() - 2;
        uint16_t crc = frame->calculateCRC(frame->frameData(), len);

        uint16_t packet_crc = (static_cast<uint16_t>(frame->frameData()[len]) << 8) | frame->frameData()[len + 1];
        if (crc != packet_crc) {
            ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_INVALID_CRC);
            // Invalid transfer frame is discarded and service aborted
            masterChannel.removeMasterRx(frame);
            masterChannel.inFramesBeforeAllFramesReceptionListRxTC.pop_front();
            return ServiceChannelNotification::RX_INVALID_CRC;
        }
        virtualChannel.waitQueueRxTC.push_back(frame);
        masterChannel.inFramesBeforeAllFramesReceptionListRxTC.pop_front();
        return ServiceChannelNotification::NO_SERVICE_EVENT;
    }
    virtualChannel.waitQueueRxTC.push_back(frame);
    masterChannel.inFramesBeforeAllFramesReceptionListRxTC.pop_front();
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

//     - Virtual Channel Reception
ServiceChannelNotification ServiceChannel::vcReceptionRxTC(uint8_t vid) {
    VirtualChannel& virtChannel = masterChannel.virtualChannels.at(vid);

    if (virtChannel.waitQueueRxTC.empty()) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_PACKETS_TO_PROCESS_IN_VC_RECEPTION_BEFORE_FARM);
        return ServiceChannelNotification::NO_PACKETS_TO_PROCESS_IN_VC_RECEPTION_BEFORE_FARM;
    }

    if (virtChannel.inFramesAfterVCReceptionRxTC.full()) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, VC_RECEPTION_BUFFER_AFTER_FARM_FULL);
        return ServiceChannelNotification::VC_RECEPTION_BUFFER_AFTER_FARM_FULL;
    }

    TransferFrameTC* frame = virtChannel.waitQueueRxTC.front();

    // FARM procedures
    virtChannel.farm.frameArrives();

    CLCW clcw =
            CLCW(0, 0, 0, 1, vid, 0, 0, 1, virtChannel.farm.lockout, virtChannel.farm.wait, virtChannel.farm.retransmit,
                 virtChannel.farm.farmBCount, 0, virtChannel.farm.receiverFrameSeqNumber);

    // add idle data
    for (uint8_t i = TmPrimaryHeaderSize; i < TmTransferFrameSize - 2 * virtChannel.frameErrorControlFieldPresent;
         i++) {
        // add idle data
        clcwTransferFrameDataBuffer[i] = idle_data[i];
    }
    TransferFrameTM clcwTransferFrame =
            TransferFrameTM(clcwTransferFrameDataBuffer, TmTransferFrameSize, virtChannel.frameCountTM, vid,
                            virtChannel.frameErrorControlFieldPresent, virtChannel.secondaryHeaderTMPresent, NoSegmentation,
                            virtChannel.synchronization, clcw.clcw, TM);
    if (!clcwTransferFrameBuffer.empty()) {
        clcwTransferFrameBuffer.pop_front();
    }
    clcwTransferFrameBuffer.push_back(clcwTransferFrame);
    clcwWaitingToBeTransmitted = true;

    // If MAP channels are implemented in this specific VC, write to the MAP buffer
    if (virtChannel.segmentHeaderPresent) {
        uint8_t mapid = frame->mapId();
        MAPChannel& mapChannel = virtChannel.mapChannels.at(mapid);
        mapChannel.inFramesAfterVCReceptionRxTC.push_back(frame);
    } else {
        virtChannel.inFramesAfterVCReceptionRxTC.push_back(frame);
    }
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

//     - Virtual Channel Extraction
ServiceChannelNotification ServiceChannel::packetExtractionTC(uint8_t vid, uint8_t* packetTarget) {
    VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));

    // If segmentation header exists, then the MAP transferFrameData extraction service needs to be called
    if (virtualChannel->segmentHeaderPresent) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, INVALID_SERVICE_CALL);
        return ServiceChannelNotification::INVALID_SERVICE_CALL;
    }

    if (virtualChannel->inFramesAfterVCReceptionRxTC.empty()) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
        return ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS;
    }

    // TODO Perhaps packetExtraction should get frames from sentQueueRxTC, and this buffer should not exist?
    // TODO If not, then why no service pops frames from it?
    TransferFrameTC* frame = virtualChannel->inFramesAfterVCReceptionRxTC.front();

    uint16_t frameSize = frame->frameLength();
    uint8_t headerSize = TcPrimaryHeaderSize; // Segment header is present
    uint8_t trailerSize = 2 * virtualChannel->frameErrorControlFieldPresent;

    memcpy(packetTarget, frame->frameData() + headerSize, frameSize - headerSize - trailerSize);

    virtualChannel->inFramesAfterVCReceptionRxTC.pop_front();
    // TODO I think we also need to delete the copy from the master buffer (look map packetTarget extraction)

    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

//     - MAP Packet Extraction
ServiceChannelNotification ServiceChannel::packetExtractionTC(uint8_t vid, uint8_t mapid, uint8_t* packet) {
    VirtualChannel& virtualChannel = masterChannel.virtualChannels.at(vid);
    // We can't call the MAP Packet Extraction service if no segmentation header is present
    if (!virtualChannel.segmentHeaderPresent) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, INVALID_SERVICE_CALL);
        return ServiceChannelNotification::INVALID_SERVICE_CALL;
    }

    MAPChannel& mapChannel = virtualChannel.mapChannels.at(mapid);

    if (mapChannel.inFramesAfterVCReceptionRxTC.empty()) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
        return ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS;
    }

    TransferFrameTC* frame = mapChannel.inFramesAfterVCReceptionRxTC.front();

    uint16_t frameSize = frame->frameLength();
    uint8_t headerSize = TcPrimaryHeaderSize + 1; // Segment header is present

    uint8_t trailerSize = 2 * virtualChannel.frameErrorControlFieldPresent;

    memcpy(packet, frame->frameData() + headerSize, frameSize - headerSize - trailerSize);

    mapChannel.inFramesAfterVCReceptionRxTC.pop_front();
    masterChannel.removeMasterRx(frame);

    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

// TM TransferFrame - Sending End (TM Tx)

//     - Utility and Debugging
uint16_t ServiceChannel::availablePacketLengthBufferTxTM(uint8_t gvcid) {
    uint8_t vid = gvcid & 0x3F;
    VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

    return vchan.packetLengthBufferTxTM.available();
}

uint16_t ServiceChannel::availablePacketBufferTxTM(uint8_t gvcid) {
    uint8_t vid = gvcid & 0x3F;
    VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

    return vchan.packetBufferTxTM.available();
}

std::pair<ServiceChannelNotification, const TransferFrameTM*> ServiceChannel::backFrameAfterVcGenerationTxTM() const {
    if (masterChannel.masterCopyTxTM.empty()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
    }
    ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, &(masterChannel.masterCopyTxTM.back()));
}

std::pair<ServiceChannelNotification, const TransferFrameTM*> ServiceChannel::frontFrameAfterAllFramesGenerationTxTM() const {
    if (masterChannel.toBeTransmittedFramesAfterAllFramesGenerationListTxTM.empty()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
    }
    ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT,
                     masterChannel.toBeTransmittedFramesAfterAllFramesGenerationListTxTM.front());
}

//     - Packet Processing
ServiceChannelNotification ServiceChannel::storePacketTxTM(uint8_t* packet, uint16_t packetLength, uint8_t vid) {
    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
        return ServiceChannelNotification::INVALID_VC_ID;
    }

    VirtualChannel* vchan = &(masterChannel.virtualChannels.at(vid));

    if (packetLength <= vchan->packetBufferTxTM.available()) {
        vchan->packetLengthBufferTxTM.push(packetLength);
        for (uint16_t i = 0; i < packetLength; i++) {
            vchan->packetBufferTxTM.push(packet[i]);
        }
        return NO_SERVICE_EVENT;
    }
    return VC_MC_FRAME_BUFFER_FULL;
}

//     - Virtual Channel Generation
ServiceChannelNotification ServiceChannel::segmentationTM(uint8_t numberOfTransferFrames, uint16_t packetLength,
                                                          uint16_t transferFrameDataFieldLength, uint8_t vid) {
    VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

    uint16_t currentTransferFrameDataFieldLength = 0;
    static uint8_t tmpData[TmTransferFrameSize] = {0};
    for (uint16_t i = 0; i < numberOfTransferFrames; i++) {
        currentTransferFrameDataFieldLength =
                packetLength > transferFrameDataFieldLength ? transferFrameDataFieldLength : packetLength;
        SegmentLengthID segmentLengthId = SegmentationMiddle;
        for (uint16_t j = 0; j < currentTransferFrameDataFieldLength; j++) {
            tmpData[j + TmPrimaryHeaderSize] = vchan.packetBufferTxTM.front();
            vchan.packetBufferTxTM.pop();
        }
        if (i == 0) {
            segmentLengthId = SegmentationStart;
        } else if (i == numberOfTransferFrames - 1) {
            segmentLengthId = SegmentaionEnd;
        }
        for (uint8_t j = 0; j < TmTrailerSize; j++) {
            if (currentTransferFrameDataFieldLength + TmPrimaryHeaderSize + i < TmTransferFrameSize) {
                tmpData[currentTransferFrameDataFieldLength + TmPrimaryHeaderSize + i] = 0;
            }
        }
        uint8_t* frameData = masterChannel.masterChannelPoolTM.allocatePacket(
                tmpData, currentTransferFrameDataFieldLength + TmPrimaryHeaderSize + TmTrailerSize);
        if (frameData == nullptr) {
            return MEMORY_POOL_FULL;
        }
        vchan.frameCountTM = (vchan.frameCountTM + 1) % 256;
        TransferFrameTM transferFrameTm =
                TransferFrameTM(frameData, currentTransferFrameDataFieldLength + TmPrimaryHeaderSize + TmTrailerSize,
                                vchan.frameCountTM, vid, vchan.frameErrorControlFieldPresent, vchan.segmentHeaderPresent,
                                segmentLengthId, vchan.synchronization, TM);
        masterChannel.masterCopyTxTM.push_back(transferFrameTm);
        masterChannel.framesAfterVcGenerationServiceTxTM.push_back(&(masterChannel.masterCopyTxTM.back()));
        packetLength -= transferFrameDataFieldLength;
    }
    vchan.packetLengthBufferTxTM.pop();
    return NO_SERVICE_EVENT;
}


ServiceChannelNotification ServiceChannel::blockingTM(uint16_t transferFrameDataFieldLength, uint16_t packetLength,
                                                      uint8_t vid) {
    VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

    uint16_t currentTransferFrameDataFieldLength = 0;
    static uint8_t tmpData[TmTransferFrameSize] = {0};

    while (currentTransferFrameDataFieldLength + packetLength <= transferFrameDataFieldLength &&
           !vchan.packetLengthBufferTxTM.empty()) {
        for (uint16_t i = currentTransferFrameDataFieldLength; i < currentTransferFrameDataFieldLength + packetLength; i++) {
            tmpData[i + TmPrimaryHeaderSize] = vchan.packetBufferTxTM.front();
            vchan.packetBufferTxTM.pop();
        }
        currentTransferFrameDataFieldLength += packetLength;
        vchan.packetLengthBufferTxTM.pop();
        packetLength = vchan.packetLengthBufferTxTM.front();
        // If blocking is disabled, stop the operation on the first transfer frame
        if (!vchan.blocking) {
            break;
        }
    }

    for (uint8_t i = 0; i < TmTrailerSize; i++) {
        if (currentTransferFrameDataFieldLength + TmPrimaryHeaderSize + i < TmTransferFrameSize) {
            tmpData[currentTransferFrameDataFieldLength + TmPrimaryHeaderSize + i] = 0;
        }
    }
    uint8_t* transferFrameData = masterChannel.masterChannelPoolTM.allocatePacket(
            tmpData, currentTransferFrameDataFieldLength + TmPrimaryHeaderSize + TmTrailerSize);
    if (transferFrameData == nullptr) {
        return MEMORY_POOL_FULL;
    }
    vchan.frameCountTM = (vchan.frameCountTM + 1) % 256;
    SegmentLengthID segmentLengthId = NoSegmentation;
    TransferFrameTM transferFrameTm =
            TransferFrameTM(transferFrameData, currentTransferFrameDataFieldLength + TmPrimaryHeaderSize + TmTrailerSize,
                            vchan.frameCountTM, vid, vchan.frameErrorControlFieldPresent, vchan.segmentHeaderPresent,
                            segmentLengthId, vchan.synchronization, TM);
    masterChannel.masterCopyTxTM.push_back(transferFrameTm);
    masterChannel.framesAfterVcGenerationServiceTxTM.push_back(&(masterChannel.masterCopyTxTM.back()));
    return NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::vcGenerationServiceTxTM(uint16_t transferFrameDataFieldLength, uint8_t vid) {
    VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

    uint16_t currentTransferFrameDataLength = 0;
    if (vchan.packetLengthBufferTxTM.empty()) {
        return PACKET_BUFFER_EMPTY;
    }
    uint16_t packetLength = vchan.packetLengthBufferTxTM.front();
    uint16_t numberOfTransferFrames =
            packetLength / transferFrameDataFieldLength + (packetLength % transferFrameDataFieldLength != 0);
    if (masterChannel.masterCopyTxTM.available() < numberOfTransferFrames) {
        return MASTER_CHANNEL_FRAME_BUFFER_FULL;
    }
    if (masterChannel.framesAfterVcGenerationServiceTxTM.available() < numberOfTransferFrames) {
        return TX_MC_FRAME_BUFFER_FULL;
    }
    if (numberOfTransferFrames <= 1) {
        return blockingTM(transferFrameDataFieldLength, packetLength, vid);
    }
    return segmentationTM(numberOfTransferFrames, packetLength, transferFrameDataFieldLength, vid);
}

//     - Master Channel Generation
ServiceChannelNotification ServiceChannel::mcGenerationRequestTxTM() {
    if (masterChannel.framesAfterVcGenerationServiceTxTM.empty()) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
        return ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS;
    }
    if (masterChannel.toBeTransmittedFramesAfterMCGenerationListTxTM.full()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, TX_MC_FRAME_BUFFER_FULL);
        return ServiceChannelNotification::TX_MC_FRAME_BUFFER_FULL;
    }
    TransferFrameTM* packet = masterChannel.framesAfterVcGenerationServiceTxTM.front();

    // Check if need to add secondary header and act accordingly
    // TODO: Process secondary headers

    // set master channel frame counter
    packet->setMasterChannelFrameCount(masterChannel.currFrameCountTM);

    // increment master channel frame counter
    masterChannel.currFrameCountTM = masterChannel.currFrameCountTM <= 254 ? masterChannel.currFrameCountTM : 0;
    masterChannel.toBeTransmittedFramesAfterMCGenerationListTxTM.push_back(packet);
    masterChannel.framesAfterVcGenerationServiceTxTM.pop_front();

    ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}


//     - All Frames Generation
ServiceChannelNotification ServiceChannel::allFramesGenerationRequestTxTM(uint8_t* frameDataTarget, uint16_t frameLength) {
    if (masterChannel.toBeTransmittedFramesAfterMCGenerationListTxTM.empty()) {
        return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
    }

    TransferFrameTM* frame = masterChannel.toBeTransmittedFramesAfterMCGenerationListTxTM.front();

    uint8_t vid = frame->virtualChannelId();
    VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

    if (vchan.frameErrorControlFieldPresent) {
        frame->appendCRC();
    }

    if (frame->getFrameLength() > frameLength) {
        return ServiceChannelNotification::RX_INVALID_LENGTH;
    }

    uint16_t frameSize = frame->getFrameLength();
    uint16_t idleDataSize = TmTransferFrameSize - frameSize;
    uint8_t trailerSize = 4 * frame->operationalControlFieldExists() + 2 * vchan.frameErrorControlFieldPresent;

    // Copy frame without the trailer
    memcpy(frameDataTarget, frame->frameData(), frameSize - trailerSize);

    // Append idle data
    memcpy(frameDataTarget + frameSize - trailerSize, idle_data, idleDataSize);

    // Append trailer
    memcpy(frameDataTarget + TmTransferFrameSize - trailerSize, frame->frameData() + frameLength - trailerSize,
           trailerSize);

    masterChannel.toBeTransmittedFramesAfterMCGenerationListTxTM.pop_front();
    // Finally, remove master copy
    masterChannel.removeMasterTx(frame);
    masterChannel.masterChannelPoolTM.deletePacket(frame->frameData(), frame->getFrameLength());

    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

// TM TransferFrame - Receiving End (TM Rx)

// Utility and Debugging
uint8_t ServiceChannel::getFrameCountTM(uint8_t vid) {
    return masterChannel.virtualChannels.at(vid).frameCountTM;
}

uint8_t ServiceChannel::getFrameCountTM() {
    return masterChannel.currFrameCountTM;
}

//     - All Frames Reception
ServiceChannelNotification ServiceChannel::allFramesReceptionRequestRxTM(uint8_t* frameData, uint16_t frameLength) {
    if (masterChannel.masterCopyRxTM.full()) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_IN_MC_FULL);
        return ServiceChannelNotification::RX_IN_MC_FULL;
    }

    uint8_t vid = (frameData[1] >> 1) & 0x7;
    // Check if Virtual channel Id does not exist in the relevant Virtual Channels map
    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        // If it doesn't, abort operation
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, INVALID_VC_ID);
        return ServiceChannelNotification::INVALID_VC_ID;
    }

    VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
    TransferFrameTM frame = TransferFrameTM(frameData, frameLength, virtualChannel->frameErrorControlFieldPresent);
    bool eccFieldExists = virtualChannel->frameErrorControlFieldPresent;

    if (virtualChannel->framesAfterMcReceptionRxTM.full()) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_IN_BUFFER_FULL);
        return ServiceChannelNotification::RX_IN_BUFFER_FULL;
    }

    if (eccFieldExists) {
        uint16_t len = frame.getFrameLength() - 2;
        uint16_t crc = frame.calculateCRC(frame.frameData(), len);

        uint16_t packet_crc =
                ((static_cast<uint16_t>(frame.frameData()[len]) << 8) & 0xFF00) | frame.frameData()[len + 1];
        if (crc != packet_crc) {
            ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_INVALID_CRC);
            // Invalid transfer frame is discarded and service aborted
            return ServiceChannelNotification::RX_INVALID_CRC;
        }
    }
    // Master Channel Reception
    uint8_t mc_lost_frames = frame.getMasterChannelFrameCount();

    // Check if master channel frames have been lost
    uint8_t mc_counter_diff = (mc_lost_frames - masterChannel.currFrameCountTM) % 0xFF;

    if (mc_counter_diff > 1) {
        // Log error that frames have been lost, but don't abort processing
        ccsdsLogNotice<uint8_t>(Rx, TypeServiceChannelNotif, MC_RX_INVALID_COUNT, mc_counter_diff);
    }
    // CLCW extraction
    std::optional<uint32_t> operationalControlField = frame.getOperationalControlField();
    if (operationalControlField.has_value() && operationalControlField.value() >> 31 == 0) {
        CLCW clcw = CLCW(operationalControlField.value());
        virtualChannel->currentlyProcessedCLCW = CLCW(clcw.getClcw());
        virtualChannel->fop.validClcwArrival();
        virtualChannel->fop.acknowledgePreviousFrames(clcw.getReportValue());
    }
    // TODO: Will we use secondary headers? If so they need to be processed here and forward to the respective service
    masterChannel.masterCopyRxTM.push_back(frame);
    TransferFrameTM* masterFrame = &(masterChannel.masterCopyRxTM.back());
    virtualChannel->framesAfterMcReceptionRxTM.push_back(masterFrame);

    return ServiceChannelNotification::NO_SERVICE_EVENT;
}


//     - Packet Extraction
ServiceChannelNotification ServiceChannel::packetExtractionRxTM(uint8_t vid, uint8_t* packetTarget) {
    VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));

    if (virtualChannel->framesAfterMcReceptionRxTM.full()) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_IN_BUFFER_FULL);
        return ServiceChannelNotification::RX_IN_BUFFER_FULL;
    }

    if (virtualChannel->framesAfterMcReceptionRxTM.empty()) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
        return ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS;
    }
    TransferFrameTM* transferFrameTm = virtualChannel->framesAfterMcReceptionRxTM.front();

    uint16_t frameSize = transferFrameTm->getFrameLength();
    uint8_t headerSize = 5 + virtualChannel->secondaryHeaderTMLength;
    uint8_t trailerSize =
            4 * transferFrameTm->operationalControlFieldExists() + 2 * virtualChannel->frameErrorControlFieldPresent;
    memcpy(packetTarget, transferFrameTm->frameData() + headerSize + 1, frameSize - headerSize - trailerSize);

    virtualChannel->framesAfterMcReceptionRxTM.pop_front();
    masterChannel.removeMasterRx(transferFrameTm);

    return ServiceChannelNotification::NO_SERVICE_EVENT;
}
