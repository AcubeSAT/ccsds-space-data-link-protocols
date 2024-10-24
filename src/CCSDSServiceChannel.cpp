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

std::pair<ServiceChannelNotification, const TransferFrameTC*> ServiceChannel::frontUnprocessedFrameTxTC(uint8_t vid) const {
    const etl::list<TransferFrameTC*, MaxReceivedUnprocessedTxTcInVirtBuffer>* vc =
            &(masterChannel.virtualChannels.at(vid).unprocessedFrameListBufferTxTC);
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

const etl::list<TransferFrameTC*, MaxReceivedUnprocessedTxTcInVirtBuffer>& ServiceChannel::getUnprocessedFramesListBuffer(uint16_t vid){
    VirtualChannel *vchan = &(masterChannel.virtualChannels.at(vid));
    return vchan->unprocessedFrameListBufferTxTC;
}

//     - MAP Packet Processing
ServiceChannelNotification ServiceChannel::segmentationTC(uint16_t maxTransferFrameDataFieldLength, uint16_t packetLength,
                                                          uint8_t vid, uint8_t mapid, ServiceType serviceType) {

    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
        return ServiceChannelNotification::INVALID_VC_ID;
    }

    VirtualChannel *vchan = &(masterChannel.virtualChannels.at(vid));

    MAPChannel *mapChannel;
    if (vchan->segmentHeaderTCPresent) {
        if (vchan->mapChannels.find(mapid) == vchan->mapChannels.end()) {
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_MAP_ID);
            return ServiceChannelNotification::INVALID_MAP_ID;
        }
        mapChannel = &(vchan->mapChannels.at(mapid));
    }

    etl::queue<uint16_t, PacketBufferTcSize> *packetLengthBufferTcTx;
    etl::queue<uint8_t, PacketBufferTcSize> *packetBufferTcTx;

    if (serviceType == ServiceType::TYPE_BC){
        packetBufferTcTx = &vchan->packetBufferTxTcTypeBC;
        packetLengthBufferTcTx = &vchan->packetLengthBufferTxTcTypeBC;
    }
    else if (vchan->segmentHeaderTCPresent){
        if (serviceType == ServiceType::TYPE_AD){
            packetBufferTcTx = &mapChannel->packetBufferTxTcTypeAD;
            packetLengthBufferTcTx = &mapChannel->packetLengthBufferTxTcTypeAD;
        }
        else { // TYPE_BD
            packetBufferTcTx = &mapChannel->packetBufferTxTcTypeBD;
            packetLengthBufferTcTx = &mapChannel->packetLengthBufferTxTcTypeBD;
        }
    }
    else {
        if (serviceType == ServiceType::TYPE_AD){
            packetBufferTcTx = &vchan->packetBufferTxTcTypeAD;
            packetLengthBufferTcTx = &vchan->packetLengthBufferTxTcTypeAD;
        }
        else { // TYPE_BD
            packetBufferTcTx = &vchan->packetBufferTxTcTypeBD;
            packetLengthBufferTcTx = &vchan->packetLengthBufferTxTcTypeBD;
        }
    }

    static uint8_t tmpData[MaxTcTransferFrameSize] = {0};
    uint16_t numberOfNewTransferFrames = 0;
    uint8_t trailerSize = vchan->frameErrorControlFieldPresent * ErrorControlFieldSize;
    uint8_t segmentHeaderOffset = (vchan->segmentHeaderTCPresent && (serviceType == ServiceType::TYPE_AD || serviceType == ServiceType::TYPE_BD))
            ? TcSegmentHeaderSize : 0;

    // Calculate amount of new transfer frames needed
    numberOfNewTransferFrames = packetLength / (maxTransferFrameDataFieldLength - segmentHeaderOffset)
                                + ((packetLength % (maxTransferFrameDataFieldLength - segmentHeaderOffset)) ? 1 :0);


    // ensure there is enough space for the new frames
    if (masterChannel.masterCopyTxTC.available() < numberOfNewTransferFrames) {
        return MASTER_CHANNEL_FRAME_BUFFER_FULL;
    }
    if (vchan->unprocessedFrameListBufferTxTC.available() < numberOfNewTransferFrames) {
        return VC_MC_FRAME_BUFFER_FULL;
    }
    if (masterChannel.masterChannelPoolTC.findFit(numberOfNewTransferFrames * (TcPrimaryHeaderSize + trailerSize + segmentHeaderOffset) + packetLength).second != NO_MC_ALERT) {
        return MEMORY_POOL_FULL;
    }

    // new frames creation
    uint16_t currentTransferFrameDataFieldLength = segmentHeaderOffset;
    for (uint16_t i = 0; i < numberOfNewTransferFrames; i++) {
        for (uint16_t j = 0; j < MaxTcTransferFrameSize; j++){  // clear up array between consecutive calls
            tmpData[j] = 0;
        }

        currentTransferFrameDataFieldLength =
                packetLength > (maxTransferFrameDataFieldLength - segmentHeaderOffset) ? maxTransferFrameDataFieldLength : packetLength + segmentHeaderOffset;

        for (uint16_t j = 0; j < currentTransferFrameDataFieldLength - segmentHeaderOffset; j++) {
            tmpData[j + TcPrimaryHeaderSize + segmentHeaderOffset] = packetBufferTcTx->front();
            packetBufferTcTx->pop();
        }

        SequenceFlags segmentLengthId = SegmentationMiddle;

        if (i == numberOfNewTransferFrames - 1){
            segmentLengthId = SegmentationEnd;
        }
        else if (i == 0) {
            segmentLengthId = SegmentationStart;
        }

        uint8_t* transferFrameData = masterChannel.masterChannelPoolTC.allocatePacket(
                tmpData, TcPrimaryHeaderSize + currentTransferFrameDataFieldLength + trailerSize);

        if (transferFrameData == nullptr) {
            return MEMORY_POOL_FULL;
        }

        TransferFrameTC transferFrameTc =
                TransferFrameTC(transferFrameData,
                                serviceType,
                                vid,
                                currentTransferFrameDataFieldLength + TcPrimaryHeaderSize + trailerSize,
                                vchan->segmentHeaderTCPresent,
                                segmentLengthId,
                                mapid,
                                currentTransferFrameDataFieldLength,
                                TC);
        masterChannel.masterCopyTxTC.push_back(transferFrameTc);
        vchan->unprocessedFrameListBufferTxTC.push_back(&(masterChannel.masterCopyTxTC.back()));
        packetLength -= maxTransferFrameDataFieldLength - segmentHeaderOffset;
    }
    packetLengthBufferTcTx->pop();
    return NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::blockingTC(uint16_t maxTransferFrameDataFieldLength, uint16_t packetLength,
                                                      uint8_t vid, uint8_t mapid, ServiceType serviceType){

    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
        return ServiceChannelNotification::INVALID_VC_ID;
    }

    VirtualChannel *vchan = &(masterChannel.virtualChannels.at(vid));

    MAPChannel *mapChannel;
    if (vchan->segmentHeaderTCPresent) {
        if (vchan->mapChannels.find(mapid) == vchan->mapChannels.end()) {
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_MAP_ID);
            return ServiceChannelNotification::INVALID_MAP_ID;
        }
        mapChannel = &(vchan->mapChannels.at(mapid));
    }

    etl::queue<uint16_t, PacketBufferTcSize> *packetLengthBufferTcTx;
    etl::queue<uint8_t, PacketBufferTcSize> *packetBufferTcTx;

    if (serviceType == ServiceType::TYPE_BC){
        packetBufferTcTx = &vchan->packetBufferTxTcTypeBC;
        packetLengthBufferTcTx = &vchan->packetLengthBufferTxTcTypeBC;
    }
    else if (vchan->segmentHeaderTCPresent){
        if (serviceType == ServiceType::TYPE_AD){
            packetBufferTcTx = &mapChannel->packetBufferTxTcTypeAD;
            packetLengthBufferTcTx = &mapChannel->packetLengthBufferTxTcTypeAD;
        }
        else { // TYPE_BD
            packetBufferTcTx = &mapChannel->packetBufferTxTcTypeBD;
            packetLengthBufferTcTx = &mapChannel->packetLengthBufferTxTcTypeBD;
        }
    }
    else {
        if (serviceType == ServiceType::TYPE_AD){
            packetBufferTcTx = &vchan->packetBufferTxTcTypeAD;
            packetLengthBufferTcTx = &vchan->packetLengthBufferTxTcTypeAD;
        }
        else { // TYPE_BD
            packetBufferTcTx = &vchan->packetBufferTxTcTypeBD;
            packetLengthBufferTcTx = &vchan->packetLengthBufferTxTcTypeBD;
        }
    }

    uint8_t trailerSize = vchan->frameErrorControlFieldPresent * ErrorControlFieldSize;
    uint8_t segmentHeaderOffset = (vchan->segmentHeaderTCPresent && (serviceType == ServiceType::TYPE_AD || serviceType == ServiceType::TYPE_BD))
                                  ? TcSegmentHeaderSize : 0;

    // ensure there is enough space for a new frame
    if (masterChannel.masterCopyTxTC.available() == 0) {
            return MASTER_CHANNEL_FRAME_BUFFER_FULL;
        }
    if (vchan->unprocessedFrameListBufferTxTC.available() == 0) {
        return VC_MC_FRAME_BUFFER_FULL;
    }
    if (masterChannel.masterChannelPoolTC.findFit(TcPrimaryHeaderSize + segmentHeaderOffset + packetLength + trailerSize).second != NO_MC_ALERT) {
        return MEMORY_POOL_FULL;
    }


    uint16_t currentTransferFrameDataFieldLength = segmentHeaderOffset;
    // @TODO change MaxTCtransfersize to vchan.maxFrameLengthTC
    static uint8_t tmpData[MaxTcTransferFrameSize] = {0};
    for (uint16_t i = 0; i< MaxTcTransferFrameSize; i++){  // clear up array between consecutive calls
        tmpData[i] = 0;
    }

    while (currentTransferFrameDataFieldLength + packetLength <= maxTransferFrameDataFieldLength &&
           !packetLengthBufferTcTx->empty()) {
        for (uint16_t i = 0; i < packetLength; i++) {
            tmpData[i + TcPrimaryHeaderSize + currentTransferFrameDataFieldLength] = packetBufferTcTx->front();
            packetBufferTcTx->pop();
        }
        currentTransferFrameDataFieldLength += packetLength;
        packetLengthBufferTcTx->pop();
        packetLength = packetLengthBufferTcTx->front();
        // If blocking is disabled, stop the operation on the first packet
        if (!vchan->blockingTC) {
            break;
        }
    }


    uint8_t* transferFrameData = masterChannel.masterChannelPoolTC.allocatePacket(
                tmpData, TcPrimaryHeaderSize + currentTransferFrameDataFieldLength + trailerSize);

    if (transferFrameData == nullptr) {
        return MEMORY_POOL_FULL;
    }

    SequenceFlags segmentLengthId = NoSegmentation;
    uint16_t firstEmptyOctet = currentTransferFrameDataFieldLength;

    TransferFrameTC transferFrameTc =
            TransferFrameTC(transferFrameData,
                            serviceType,
                            vid,
                            currentTransferFrameDataFieldLength + TcPrimaryHeaderSize + trailerSize,
                            vchan->segmentHeaderTCPresent,
                            segmentLengthId,
                            mapid,
                            firstEmptyOctet,
                            TC);

    masterChannel.masterCopyTxTC.push_back(transferFrameTc);
    vchan->unprocessedFrameListBufferTxTC.push_back(&(masterChannel.masterCopyTxTC.back()));

    return NO_SERVICE_EVENT;
}


ServiceChannelNotification ServiceChannel::storePacketTxTC(uint8_t *packet, uint16_t packetLength, uint8_t vid, uint8_t mapid,
                                                           ServiceType serviceType) {
    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
        return ServiceChannelNotification::INVALID_VC_ID;
    }

    VirtualChannel *vchan = &(masterChannel.virtualChannels.at(vid));

    MAPChannel *mapChannel;
    if (vchan->segmentHeaderTCPresent) {
        if (vchan->mapChannels.find(mapid) == vchan->mapChannels.end()) {
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_MAP_ID);
            return ServiceChannelNotification::INVALID_MAP_ID;
        }
        mapChannel = &(vchan->mapChannels.at(mapid));
    }

    etl::queue<uint16_t, PacketBufferTcSize> *packetLengthBufferTcTx;
    etl::queue<uint8_t, PacketBufferTcSize> *packetBufferTcTx;

    if (serviceType == ServiceType::TYPE_BC){
        packetBufferTcTx = &(vchan->packetBufferTxTcTypeBC);
        packetLengthBufferTcTx = &(vchan->packetLengthBufferTxTcTypeBC);
    }
    else if (vchan->segmentHeaderTCPresent){
        if (serviceType == ServiceType::TYPE_AD){
            packetBufferTcTx = &(mapChannel->packetBufferTxTcTypeAD);
            packetLengthBufferTcTx = &(mapChannel->packetLengthBufferTxTcTypeAD);
        }
        else { // TYPE_BD
            packetBufferTcTx = &(mapChannel->packetBufferTxTcTypeBD);
            packetLengthBufferTcTx = &(mapChannel->packetLengthBufferTxTcTypeBD);
        }
    }
    else {
        if (serviceType == ServiceType::TYPE_AD){
            packetBufferTcTx = &(vchan->packetBufferTxTcTypeAD);
            packetLengthBufferTcTx = &(vchan->packetLengthBufferTxTcTypeAD);
        }
        else { // TYPE_BD
            packetBufferTcTx = &(vchan->packetBufferTxTcTypeBD);
            packetLengthBufferTcTx = &(vchan->packetLengthBufferTxTcTypeBD);
        }
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

ServiceChannelNotification ServiceChannel::packetProcessingRequestTxTC(uint8_t vid, uint8_t mapid, uint8_t maxTransferFrameDataFieldLength, ServiceType serviceType) {

    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
        return ServiceChannelNotification::INVALID_VC_ID;
    }

    VirtualChannel *vchan = &(masterChannel.virtualChannels.at(vid));

    MAPChannel *mapChannel;
    bool segmentationAllowed = false;
    if (vchan->segmentHeaderTCPresent) {
        if (vchan->mapChannels.find(mapid) == vchan->mapChannels.end()) {
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_MAP_ID);
            return ServiceChannelNotification::INVALID_MAP_ID;
        }
        mapChannel = &(vchan->mapChannels.at(mapid));
        segmentationAllowed = mapChannel->segmentationTC && (serviceType != ServiceType::TYPE_BC);
    }

    if (maxTransferFrameDataFieldLength >
    MaxTcTransferFrameSize - TcPrimaryHeaderSize - vchan->frameErrorControlFieldPresent * ErrorControlFieldSize) {
        return ServiceChannelNotification::INVALID_INPUT;
    }

    etl::queue<uint16_t, PacketBufferTcSize> *packetLengthBufferTcTx;
    etl::queue<uint8_t, PacketBufferTcSize> *packetBufferTcTx;

    if (serviceType == ServiceType::TYPE_BC){
        packetBufferTcTx = &vchan->packetBufferTxTcTypeBC;
        packetLengthBufferTcTx = &vchan->packetLengthBufferTxTcTypeBC;
    }
    else if (vchan->segmentHeaderTCPresent){
        if (serviceType == ServiceType::TYPE_AD){
            packetBufferTcTx = &mapChannel->packetBufferTxTcTypeAD;
            packetLengthBufferTcTx = &mapChannel->packetLengthBufferTxTcTypeAD;
        }
        else { // TYPE_BD
            packetBufferTcTx = &mapChannel->packetBufferTxTcTypeBD;
            packetLengthBufferTcTx = &mapChannel->packetLengthBufferTxTcTypeBD;
        }
    }
    else {
        if (serviceType == ServiceType::TYPE_AD){
            packetBufferTcTx = &vchan->packetBufferTxTcTypeAD;
            packetLengthBufferTcTx = &vchan->packetLengthBufferTxTcTypeAD;
        }
        else { // TYPE_BD
            packetBufferTcTx = &vchan->packetBufferTxTcTypeBD;
            packetLengthBufferTcTx = &vchan->packetLengthBufferTxTcTypeBD;
        }
    }

    ServiceChannelNotification notif = NO_SERVICE_EVENT;

    if (packetLengthBufferTcTx->empty()) {
        return PACKET_BUFFER_EMPTY;
    }

    while (!packetLengthBufferTcTx->empty()){
        uint16_t packetLength = packetLengthBufferTcTx->front();

        if (packetLength <= maxTransferFrameDataFieldLength - 1 * vchan->segmentHeaderTCPresent){
            notif = blockingTC(maxTransferFrameDataFieldLength, packetLength, vid, mapid, serviceType);
        }
        else if (segmentationAllowed){
            notif = segmentationTC(maxTransferFrameDataFieldLength, packetLength, vid, mapid, serviceType);
        }
        else{
            // packet too large to fit in a frame and segmentation is not allowed -> discard and notify
            for (uint16_t i = 0; i < packetLength; i++){
                packetBufferTcTx->pop();
            }
            packetLengthBufferTcTx->pop();
            notif = PACKET_EXCEEDS_MAX_SIZE;
        }

        if (notif != NO_SERVICE_EVENT){
            break;
        }
    }
    return notif;
}

//     - Virtual Channel Generation
ServiceChannelNotification ServiceChannel::vcGenerationRequestTxTC(uint8_t vid) {
    VirtualChannel& virt_channel = masterChannel.virtualChannels.at(vid);
    if (virt_channel.unprocessedFrameListBufferTxTC.empty()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
    }

    if (masterChannel.outFramesBeforeAllFramesGenerationListTxTC.full()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, TX_MC_FRAME_BUFFER_FULL);
        return ServiceChannelNotification::TX_MC_FRAME_BUFFER_FULL;
    }

    TransferFrameTC& frame = *virt_channel.unprocessedFrameListBufferTxTC.front();
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
    virt_channel.unprocessedFrameListBufferTxTC.pop_front();
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
CLCW ServiceChannel::getClcwInBuffer(uint8_t vid) {
    return masterChannel.virtualChannels.at(0).generatedClcwBuffer.front();
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

    uint8_t vid = frame->getVirtualChannelId();
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

    uint8_t vid = transferFrameTc.getVirtualChannelId();
    uint8_t mapid = transferFrameTc.getMapId();

    // Check if Virtual Channel Id does not exist in the relevant Virtual Channels map
    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        // If it doesn't, abort operation
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
        return ServiceChannelNotification::INVALID_VC_ID;
    }

    VirtualChannel* vchan = &(masterChannel.virtualChannels.at(vid));

    // If Segment Header present, check if MAP channel Id does not exist in the relevant MAP Channels map
    if (vchan->segmentHeaderTCPresent && (vchan->mapChannels.find(mapid) == vchan->mapChannels.end())) {
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
    VirtualChannel& virtualChannel = masterChannel.virtualChannels.at(frame->getVirtualChannelId());

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
    if (frame->getSpacecraftId() != SpacecraftIdentifier) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_INVALID_SCID);
        return ServiceChannelNotification::RX_INVALID_SCID;
    }

    // TransferFrameTC length is checked upon storing the transferFrameData in the MC

    // If present in channel, check if CRC is valid
    bool eccFieldExists = virtualChannel.frameErrorControlFieldPresent;

    if (eccFieldExists) {
        uint16_t len = frame->getFrameLength() - 2;
        uint16_t crc = frame->calculateCRC(frame->getFrameData(), len);

        uint16_t packet_crc = (static_cast<uint16_t>(frame->getFrameData()[len]) << 8) | frame->getFrameData()[len + 1];
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

    if (!virtChannel.generatedClcwBuffer.empty()) {
        virtChannel.generatedClcwBuffer.pop_front();
    }
    virtChannel.generatedClcwBuffer.push_back(clcw);
    virtChannel.clcwWaitingToBeTransmitted = true;

    // If MAP channels are implemented in this specific VC, write to the MAP buffer
    if (virtChannel.segmentHeaderTCPresent) {
        uint8_t mapid = frame->getMapId();
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
    if (virtualChannel->segmentHeaderTCPresent) {
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

    uint16_t frameSize = frame->getFrameLength();
    uint8_t headerSize = TcPrimaryHeaderSize; // Segment header is not present
    uint8_t trailerSize = 2 * virtualChannel->frameErrorControlFieldPresent;

    memcpy(packetTarget, frame->getFrameData() + headerSize, frameSize - headerSize - trailerSize);

    virtualChannel->inFramesAfterVCReceptionRxTC.pop_front();
    // TODO I think we also need to delete the copy from the master buffer (look map packetTarget extraction)

    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

//     - MAP Packet Extraction
ServiceChannelNotification ServiceChannel::packetExtractionTC(uint8_t vid, uint8_t mapid, uint8_t* packet) {
    VirtualChannel& virtualChannel = masterChannel.virtualChannels.at(vid);
    // We can't call the MAP Packet Extraction service if no segmentation header is present
    if (!virtualChannel.segmentHeaderTCPresent) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, INVALID_SERVICE_CALL);
        return ServiceChannelNotification::INVALID_SERVICE_CALL;
    }

    MAPChannel& mapChannel = virtualChannel.mapChannels.at(mapid);

    if (mapChannel.inFramesAfterVCReceptionRxTC.empty()) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
        return ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS;
    }

    TransferFrameTC* frame = mapChannel.inFramesAfterVCReceptionRxTC.front();

    uint16_t frameSize = frame->getFrameLength();
    uint8_t headerSize = TcPrimaryHeaderSize + 1; // Segment header is present

    uint8_t trailerSize = 2 * virtualChannel.frameErrorControlFieldPresent;

    memcpy(packet, frame->getFrameData() + headerSize, frameSize - headerSize - trailerSize);

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
    if (masterChannel.framesAfterVcGenerationServiceTxTM.empty()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
    }
    ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, masterChannel.framesAfterVcGenerationServiceTxTM.back());
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
        vchan->packetLengthBufferTxTM.push_back(packetLength);
        for (uint16_t i = 0; i < packetLength; i++) {
            vchan->packetBufferTxTM.push_back(packet[i]);
        }
        return NO_SERVICE_EVENT;
    }
    return VC_MC_FRAME_BUFFER_FULL;
}

//     - Virtual Channel Generation
ServiceChannelNotification ServiceChannel::segmentationTM(TransferFrameTM* prevFrame, uint16_t transferFrameDataFieldLength,
                                                          uint16_t packetLength, uint8_t vid) {
    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
        return ServiceChannelNotification::INVALID_VC_ID;
    }

    VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

    static uint8_t tmpData[TmTransferFrameSize] = {0};
    uint16_t  numberOfNewTransferFrames = 0;
    uint16_t prevFrameCapacity = prevFrame ==
            nullptr ? 0 : transferFrameDataFieldLength - prevFrame->getFirstDataFieldEmptyOctet();

    // Calculate amount of new transfer frames needed
    if (prevFrame == nullptr){
        numberOfNewTransferFrames = packetLength / transferFrameDataFieldLength + ((packetLength % transferFrameDataFieldLength) ? 1:0);
    }
    else {
        numberOfNewTransferFrames = (packetLength - prevFrameCapacity) / transferFrameDataFieldLength +
                                    ((packetLength - prevFrameCapacity) % transferFrameDataFieldLength ? 1 : 0);
    }

    // ensure there is enough space for the new frames
    if (masterChannel.masterCopyTxTM.available() < numberOfNewTransferFrames) {
        return MASTER_CHANNEL_FRAME_BUFFER_FULL;
    }
    if (masterChannel.framesAfterVcGenerationServiceTxTM.available() < numberOfNewTransferFrames) {
        return TX_MC_FRAME_BUFFER_FULL;
    }
    if (masterChannel.masterChannelPoolTM.findFit(numberOfNewTransferFrames * (TmPrimaryHeaderSize + transferFrameDataFieldLength +
                                                   vchan.operationalControlFieldTMPresent * TmOperationalControlFieldSize +
                                                   vchan.frameErrorControlFieldPresent * ErrorControlFieldSize)).second != NO_MC_ALERT) {
        return MEMORY_POOL_FULL;
    }

    uint8_t trailerSize = vchan.frameErrorControlFieldPresent * ErrorControlFieldSize + vchan.operationalControlFieldTMPresent * TmOperationalControlFieldSize;
    // fill previous frame
    if (prevFrame != nullptr){
        for (uint16_t i = 0; i< TmTransferFrameSize; i++){  // clear up array between consecutive calls
            tmpData[i] = 0;
        }

        for (uint16_t i = 0; i < TmPrimaryHeaderSize + transferFrameDataFieldLength - prevFrameCapacity; i++){
            tmpData[i] = prevFrame->getFrameData()[i];
        }

        for (uint16_t i = 0; i < prevFrameCapacity; i++){
            tmpData[i + TmPrimaryHeaderSize + transferFrameDataFieldLength - prevFrameCapacity] = vchan.packetBufferTxTM.front();
            vchan.packetBufferTxTM.pop_front();
        }
        prevFrame->setFrameData(tmpData, TmPrimaryHeaderSize + transferFrameDataFieldLength);
        prevFrame->setFirstDataFieldEmptyOctet(transferFrameDataFieldLength);
        packetLength -= prevFrameCapacity;
    }

    // new frames creation
    uint16_t currentTransferFrameDataFieldLength = 0;
    for (uint16_t i = 0; i < numberOfNewTransferFrames; i++) {
        for (uint16_t j = 0; j < TmTransferFrameSize; j++){  // clear up array between consecutive calls
            tmpData[j] = 0;
        }

        currentTransferFrameDataFieldLength =
                packetLength > transferFrameDataFieldLength ? transferFrameDataFieldLength : packetLength;

        for (uint16_t j = 0; j < currentTransferFrameDataFieldLength; j++) {
            tmpData[j + TmPrimaryHeaderSize] = vchan.packetBufferTxTM.front();
            vchan.packetBufferTxTM.pop_front();
        }

        uint16_t firstHeaderPointer = TmNoPacketStartFirstHeaderPointer;

        if (prevFrame != nullptr && i == numberOfNewTransferFrames - 1){
            firstHeaderPointer = (packetLength == transferFrameDataFieldLength) ? TmNoPacketStartFirstHeaderPointer : packetLength;
        }
        else if (prevFrame == nullptr) {
            if (i == 0) {
                firstHeaderPointer = 0;
            } else if (i == numberOfNewTransferFrames - 1) {
                firstHeaderPointer = (packetLength == transferFrameDataFieldLength) ? TmNoPacketStartFirstHeaderPointer : packetLength;
            }
        }

        uint8_t* transferFrameData = masterChannel.masterChannelPoolTM.allocatePacket(
                tmpData, TmPrimaryHeaderSize + transferFrameDataFieldLength + trailerSize);

        if (transferFrameData == nullptr) {
            return MEMORY_POOL_FULL;
        }
        vchan.frameCountTM = (vchan.frameCountTM + 1) % 256;
        TransferFrameTM transferFrameTm =
                TransferFrameTM(transferFrameData,
                                transferFrameDataFieldLength + TmPrimaryHeaderSize + trailerSize,
                                vid,
                                vchan.operationalControlFieldTMPresent,
                                vchan.frameCountTM,
                                vchan.secondaryHeaderTMPresent,
                                vchan.synchronization,
                                SegmentLengthIdentifier,
                                PacketOrderFlag, firstHeaderPointer,
                                vchan.frameErrorControlFieldPresent,
                                currentTransferFrameDataFieldLength,
                                TM);

        // add clcw to the operational control field
        if (vchan.clcwWaitingToBeTransmitted && vchan.operationalControlFieldTMPresent){
            transferFrameTm.setOperationalControlField(vchan.generatedClcwBuffer.front().clcw);
            vchan.generatedClcwBuffer.pop_front();
        }

        masterChannel.masterCopyTxTM.push_back(transferFrameTm);
        masterChannel.framesAfterVcGenerationServiceTxTM.push_back(&(masterChannel.masterCopyTxTM.back()));
        packetLength -= transferFrameDataFieldLength;
    }
    vchan.packetLengthBufferTxTM.pop_front();
    return NO_SERVICE_EVENT;
}


ServiceChannelNotification ServiceChannel::blockingTM(TransferFrameTM* prevFrame, uint16_t transferFrameDataFieldLength, uint16_t packetLength,
                                                      uint8_t vid) {
    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
        return ServiceChannelNotification::INVALID_VC_ID;
    }

    VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

    if (prevFrame == nullptr){
        // ensure there is enough space for a new frame
        if (masterChannel.masterCopyTxTM.available() == 0) {
            return MASTER_CHANNEL_FRAME_BUFFER_FULL;
        }
        if (masterChannel.framesAfterVcGenerationServiceTxTM.available() == 0) {
            return TX_MC_FRAME_BUFFER_FULL;
        }
        if (masterChannel.masterChannelPoolTM.findFit(TmPrimaryHeaderSize + transferFrameDataFieldLength +
                                                      vchan.operationalControlFieldTMPresent * TmOperationalControlFieldSize +
                                                      vchan.frameErrorControlFieldPresent * ErrorControlFieldSize).second != NO_MC_ALERT) {
            return MEMORY_POOL_FULL;
        }
    }

    uint16_t currentTransferFrameDataFieldLength = (prevFrame == nullptr) ? 0 : prevFrame->getFirstDataFieldEmptyOctet();
    static uint8_t tmpData[TmTransferFrameSize] = {0};
    for (uint16_t i = 0; i< TmTransferFrameSize; i++){  // clear up array between consecutive calls
        tmpData[i] = 0;
    }

    while (currentTransferFrameDataFieldLength + packetLength <= transferFrameDataFieldLength &&
           !vchan.packetLengthBufferTxTM.empty()) {
        for (uint16_t i = 0; i < packetLength; i++) {
            tmpData[i + TmPrimaryHeaderSize + currentTransferFrameDataFieldLength] = vchan.packetBufferTxTM.front();
            vchan.packetBufferTxTM.pop_front();
        }
        currentTransferFrameDataFieldLength += packetLength;
        vchan.packetLengthBufferTxTM.pop_front();
        packetLength = vchan.packetLengthBufferTxTM.front();
        // If blocking is disabled, stop the operation on the first packet
        if (!vchan.blockingTM) {
            break;
        }
    }

    uint8_t trailerSize = vchan.operationalControlFieldTMPresent * TmOperationalControlFieldSize +
                          vchan.frameErrorControlFieldPresent * ErrorControlFieldSize;
    if (prevFrame == nullptr) {
        uint8_t *transferFrameData = masterChannel.masterChannelPoolTM.allocatePacket(
                tmpData, TmPrimaryHeaderSize + transferFrameDataFieldLength + trailerSize);
        if (transferFrameData == nullptr) {
            return MEMORY_POOL_FULL;
        }
        vchan.frameCountTM = (vchan.frameCountTM + 1) % 256;
        uint16_t firstEmptyOctet = currentTransferFrameDataFieldLength;
        TransferFrameTM transferFrameTm =
                TransferFrameTM(transferFrameData,
                                transferFrameDataFieldLength + TmPrimaryHeaderSize + trailerSize,
                                vid,
                                vchan.operationalControlFieldTMPresent,
                                vchan.frameCountTM,
                                vchan.secondaryHeaderTMPresent,
                                vchan.synchronization,
                                PacketOrderFlag,
                                SegmentLengthIdentifier,
                                0,
                                vchan.frameErrorControlFieldPresent,
                                firstEmptyOctet,
                                TM);

        // add clcw to the operational control field
      //  if (vchan.clcwWaitingToBeTransmitted && vchan.operationalControlFieldTMPresent){
        if (vchan.operationalControlFieldTMPresent){
            transferFrameTm.setOperationalControlField(vchan.generatedClcwBuffer.front().clcw);
            vchan.generatedClcwBuffer.pop_front();
        }

        masterChannel.masterCopyTxTM.push_back(transferFrameTm);
        masterChannel.framesAfterVcGenerationServiceTxTM.push_back(&(masterChannel.masterCopyTxTM.back()));
    }
    else {
        for (uint16_t i = 0; i< TmPrimaryHeaderSize + prevFrame->getFirstDataFieldEmptyOctet(); i++){
            tmpData[i] = prevFrame->getFrameData()[i];
        }
        prevFrame->setFirstDataFieldEmptyOctet(currentTransferFrameDataFieldLength);
        prevFrame->setFrameData(tmpData, TmPrimaryHeaderSize + prevFrame->getFirstDataFieldEmptyOctet());
    }
    return NO_SERVICE_EVENT;
}

std::pair<ServiceChannelNotification, bool> ServiceChannel::generateIdleSpacePacket(uint8_t vid, TransferFrameTM* lastProcessedFrame, uint16_t transferFrameDataFieldLength, bool lastPacketPlacedIdle) {

    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
        return std::make_pair(ServiceChannelNotification::INVALID_VC_ID, false);
    }

    VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

    if (lastProcessedFrame == nullptr) {
        return std::make_pair(ServiceChannelNotification::NO_SERVICE_EVENT, false);
    }

    uint16_t firstDataFieldEmptyOctet = lastProcessedFrame->getFirstDataFieldEmptyOctet();
    if (firstDataFieldEmptyOctet == transferFrameDataFieldLength){
        return std::make_pair(NO_SERVICE_EVENT, false);
    }

    // The idle packet data length field, reduced by 1 (see p. 4.1.3.5 of space packet protocol)
    uint16_t idlePacketDataLength;
    uint16_t remainingSpace = transferFrameDataFieldLength - firstDataFieldEmptyOctet;
    if (vchan.packetLengthBufferTxTM.empty()) {
        if (remainingSpace > packetPrimaryHeaderLength + 1){
            // The idle packet can fit in the transfer frame (packetPrimaryHeaderLength + 1 is the minimum size).
            idlePacketDataLength = remainingSpace - packetPrimaryHeaderLength - 1;
        }
        else {
            // The idle packet cannot fit (will be segmented).
            // It must be large enough to fully cover the second frame (since packet buffer is empty).
            idlePacketDataLength = (remainingSpace + transferFrameDataFieldLength) - packetPrimaryHeaderLength - 1;
        }

        for (uint8_t i = 0; i < packetPrimaryHeaderLength - 2; i++){
            vchan.packetBufferTxTM.push_back(packetPrimaryHeader[i]);

        }

        vchan.packetBufferTxTM.push_back(static_cast<uint8_t>(idlePacketDataLength >> 8));
        vchan.packetBufferTxTM.push_back(static_cast<uint8_t>(idlePacketDataLength));

        for (uint16_t i = 0; i < idlePacketDataLength + 1; i++) {
            vchan.packetBufferTxTM.push_back(idle_data[i]);
        }

        vchan.packetLengthBufferTxTM.push_back(packetPrimaryHeaderLength + idlePacketDataLength + 1);
    }
    else {
        // Packets do exist, but due to blocking/segmentation permissions they may not be able to be placed.In this case, an idle packet
        // needs to be pushed to the front of the queue, in order to be processed first.

        bool nonIdleCanFit = vchan.packetLengthBufferTxTM.front() <= remainingSpace;
        if ((nonIdleCanFit && lastPacketPlacedIdle) || (nonIdleCanFit && !lastPacketPlacedIdle && vchan.blockingTM) || (!nonIdleCanFit && vchan.segmentationTM)){
            return std::make_pair(NO_SERVICE_EVENT, false);
        }

        if (remainingSpace > packetPrimaryHeaderLength + 1){
            // The idle packet can fit in the transfer frame (packetPrimaryHeaderLength + 1 is the minimum size).
            idlePacketDataLength = remainingSpace - packetPrimaryHeaderLength - 1;
        }
        else {
            // The idle packet cannot fit (will be segmented).
            // It must have the smallest possible length, in order to not waste data field space from the second frame.
            idlePacketDataLength = 0;
        }


        for (uint16_t i = 0; i < idlePacketDataLength + 1; i++) {
            vchan.packetBufferTxTM.push_front(idle_data[i]);
        }

        vchan.packetBufferTxTM.push_front((static_cast<uint8_t>(idlePacketDataLength)));
        vchan.packetBufferTxTM.push_front((static_cast<uint8_t>(idlePacketDataLength) >> 8));


        for (int8_t i = packetPrimaryHeaderLength - 3; i >= 0; i--){
            vchan.packetBufferTxTM.push_front(packetPrimaryHeader[i]);
        }

        vchan.packetLengthBufferTxTM.push_front(packetPrimaryHeaderLength + idlePacketDataLength + 1);
    }

    return std::make_pair(NO_SERVICE_EVENT, true);
}

ServiceChannelNotification ServiceChannel::vcGenerationServiceTxTM(uint16_t transferFrameDataFieldLength, uint8_t vid) {
    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
        return ServiceChannelNotification::INVALID_VC_ID;
    }

    VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);
    ServiceChannelNotification notif = NO_SERVICE_EVENT;

    // generate OID frame
    if (vchan.packetLengthBufferTxTM.empty()) {
        if (masterChannel.masterChannelPoolTM.findFit(TmPrimaryHeaderSize + transferFrameDataFieldLength +
                                                      vchan.operationalControlFieldTMPresent * TmOperationalControlFieldSize +
                                                      vchan.frameErrorControlFieldPresent * ErrorControlFieldSize).second != NO_MC_ALERT) {
            return MEMORY_POOL_FULL;
        }

        static uint8_t tmpData[TmTransferFrameSize] = {0};

        for (uint16_t i = 0; i < TmTransferFrameSize; i++){
            tmpData[i] = 0;
        }
        std::memcpy(tmpData + TmPrimaryHeaderSize, idle_data, transferFrameDataFieldLength);

        uint8_t *transferFrameData = masterChannel.masterChannelPoolTM.allocatePacket(
                tmpData,
                TmPrimaryHeaderSize + transferFrameDataFieldLength +
                TmOperationalControlFieldSize * vchan.operationalControlFieldTMPresent +
                ErrorControlFieldSize * vchan.frameErrorControlFieldPresent);
        vchan.frameCountTM = (vchan.frameCountTM + 1) % 256;
        TransferFrameTM frameOID =
                TransferFrameTM(transferFrameData,
                                TmPrimaryHeaderSize + transferFrameDataFieldLength +
                                TmOperationalControlFieldSize * vchan.operationalControlFieldTMPresent +
                                ErrorControlFieldSize * vchan.frameErrorControlFieldPresent,
                                vid,
                                vchan.operationalControlFieldTMPresent,
                                vchan.frameCountTM,
                                vchan.secondaryHeaderTMPresent,
                                vchan.synchronization,
                                PacketOrderFlag,
                                SequenceFlags::NoSegmentation,
                                TmOIDFrameFirstHeaderPointer,
                                vchan.frameErrorControlFieldPresent,
                                transferFrameDataFieldLength,
                                TM);

        // add clcw to the operational control field
//        if (vchan.clcwWaitingToBeTransmitted && vchan.operationalControlFieldTMPresent){
        if (vchan.operationalControlFieldTMPresent) {
            uint32_t test = vchan.generatedClcwBuffer.front().clcw;
            frameOID.setOperationalControlField(vchan.generatedClcwBuffer.front().clcw);
            vchan.generatedClcwBuffer.pop_front();
        }

        masterChannel.masterCopyTxTM.push_back(frameOID);
        masterChannel.framesAfterVcGenerationServiceTxTM.push_back(&(masterChannel.masterCopyTxTM.back()));
        return PACKET_BUFFER_EMPTY;
    }

    bool idlePacketQueued = false; // Flag indicating if the first packet in the queue is an idle space packet
    bool lastPacketPlacedIdle = false; // Flag indicating if the last placed packet in a frame is an idle space packet
    while (!vchan.packetLengthBufferTxTM.empty()){
        uint16_t packetLength = vchan.packetLengthBufferTxTM.front();
        TransferFrameTM* prevFrame = nullptr;
        etl::list<TransferFrameTM*, MaxReceivedUnprocessedTxTmInVirtBuffer> *TmBuffer = &(masterChannel.framesAfterVcGenerationServiceTxTM);

        if (!TmBuffer->empty()) {
            for (auto rit = TmBuffer->crbegin(); rit != TmBuffer->crend(); ++rit) {
                if ((*rit)-> getFirstHeaderPointer() != TmOIDFrameFirstHeaderPointer) {
                    prevFrame = *rit;
                    break;
                }
            }
        }

        if (prevFrame == nullptr || (prevFrame->getFirstDataFieldEmptyOctet() == transferFrameDataFieldLength)){
            if (packetLength <= transferFrameDataFieldLength){
                uint8_t available = vchan.packetLengthBufferTxTM.available();
                notif = blockingTM(nullptr, transferFrameDataFieldLength, packetLength, vid);
                lastPacketPlacedIdle = idlePacketQueued && (available - vchan.packetLengthBufferTxTM.available() == 1);
            }
            else if (vchan.segmentationTM || idlePacketQueued){
                notif = segmentationTM(nullptr, transferFrameDataFieldLength, packetLength, vid);
                lastPacketPlacedIdle = idlePacketQueued;
            }
            else {
                // packet too large to fit in a frame and segmentation is not allowed -> discard
                for (uint16_t i = 0; i < packetLength; i++){
                    vchan.packetBufferTxTM.pop_front();
                }
                vchan.packetLengthBufferTxTM.pop_front();
                notif = PACKET_EXCEEDS_MAX_SIZE;
            }
        }
        else{
            if (packetLength <= transferFrameDataFieldLength - prevFrame->getFirstDataFieldEmptyOctet()){
                uint8_t available = vchan.packetLengthBufferTxTM.available();
                notif = blockingTM(prevFrame, transferFrameDataFieldLength, packetLength, vid);
                lastPacketPlacedIdle = idlePacketQueued && (available - vchan.packetLengthBufferTxTM.available() == 1);
            }
            else {
                notif = segmentationTM(prevFrame, transferFrameDataFieldLength, packetLength, vid);
                lastPacketPlacedIdle = idlePacketQueued;
            }
        }

        // generate idle frames if needed
        idlePacketQueued = generateIdleSpacePacket(vid, masterChannel.framesAfterVcGenerationServiceTxTM.back(), transferFrameDataFieldLength, lastPacketPlacedIdle).second;

        if (notif != NO_SERVICE_EVENT){
            break;
        }

    }
    return notif;
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
    TransferFrameTM* frame = masterChannel.framesAfterVcGenerationServiceTxTM.front();

    // TODO: Process secondary headers here (if implemented)

    // set master channel frame counter
    masterChannel.currFrameCountTM = (masterChannel.currFrameCountTM + 1) % 256;
    frame->setMasterChannelFrameCount(masterChannel.currFrameCountTM);

    masterChannel.toBeTransmittedFramesAfterMCGenerationListTxTM.push_back(frame);
    masterChannel.framesAfterVcGenerationServiceTxTM.pop_front();

    ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}


//     - All Frames Generation
ServiceChannelNotification ServiceChannel::allFramesGenerationRequestTxTM(uint8_t* frameDataTarget, uint16_t& frameLength) {
    if (masterChannel.toBeTransmittedFramesAfterMCGenerationListTxTM.empty()) {
        return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
    }

    TransferFrameTM* frame = masterChannel.toBeTransmittedFramesAfterMCGenerationListTxTM.front();

    uint8_t vid = frame->getVirtualChannelId();
    VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

    if (vchan.frameErrorControlFieldPresent) {
        frame->appendCRC();
    }

    memcpy(frameDataTarget, frame->getFrameData(), frame->getFrameLength());
    frameLength = frame->getFrameLength();

    masterChannel.toBeTransmittedFramesAfterMCGenerationListTxTM.pop_front();
    // Finally, remove master copy
    masterChannel.removeMasterTx(frame);
    masterChannel.masterChannelPoolTM.deletePacket(frame->getFrameData(), frame->getFrameLength());

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
    uint8_t trailerSize = virtualChannel->operationalControlFieldTMPresent * TmOperationalControlFieldSize +
                          virtualChannel->frameErrorControlFieldPresent * ErrorControlFieldSize;
    TransferFrameTM frame = TransferFrameTM(frameData, frameLength, virtualChannel->frameErrorControlFieldPresent,
                                            frameLength - TmPrimaryHeaderSize - trailerSize);
    bool eccFieldExists = virtualChannel->frameErrorControlFieldPresent;

    if (virtualChannel->framesAfterMcReceptionRxTM.full()) {
        ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_IN_BUFFER_FULL);
        return ServiceChannelNotification::RX_IN_BUFFER_FULL;
    }

    if (eccFieldExists) {
        uint16_t len = frame.getFrameLength() - 2;
        uint16_t crc = frame.calculateCRC(frame.getFrameData(), len);

        uint16_t packet_crc =
                ((static_cast<uint16_t>(frame.getFrameData()[len]) << 8) & 0xFF00) | frame.getFrameData()[len + 1];
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

    // TODO: There should be a TC Tx service that takes as input the clcw stream from yacms and places them in receivedClcwBuffer
    // CLCW extraction
    std::optional<uint32_t> operationalControlField = frame.getOperationalControlField();
    if (operationalControlField.has_value() && operationalControlField.value() >> 31 == 0) {
        CLCW clcw = CLCW(operationalControlField.value());
        virtualChannel->receivedClcwBuffer.push_back(CLCW(clcw.getClcw()));
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
            4 * transferFrameTm->getOperationalControlFieldFlag() + 2 * virtualChannel->frameErrorControlFieldPresent;
    memcpy(packetTarget, transferFrameTm->getFrameData() + headerSize + 1, frameSize - headerSize - trailerSize);

    virtualChannel->framesAfterMcReceptionRxTM.pop_front();
    masterChannel.removeMasterRx(transferFrameTm);

    return ServiceChannelNotification::NO_SERVICE_EVENT;
}
