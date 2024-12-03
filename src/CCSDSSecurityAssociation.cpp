#include "CCSDSSecurityAssociation.hpp"
#include "tinycrypt/hmac.h"

uint8_t SecurityAssociation::getSecurityHeaderLength() const {
    return securityParameterIndexLength + initializationVectorLength + sequenceNumberFieldLength + padFieldLength;
}

uint8_t SecurityAssociation::getSecurityTrailerLength() const {
    return macFieldLength;
}

void SecurityAssociation::applySecurityTC(TransferFrameTC& frameTc, uint16_t transferFrameDataFieldLength,uint8_t vid, uint8_t mapid) {

    // ensure virtual channel exists and is associated with this SA
    if ((masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) ||
         (associatedChannels.find(vid) == associatedChannels.end())) {
        return;
    }
    VirtualChannel *vchan = &(masterChannel.virtualChannels.at(vid));

    // ensure MAP channel exists (if this virtual channel has map channels) and is associated with this SA
    MAPChannel *mapChannel;
    if (vchan->segmentHeaderTCPresent) {
        if (vchan->mapChannels.find(mapid) == vchan->mapChannels.end()) {
            return;
        }

        bool found = false;
        for (auto it = associatedChannels.at(vid).begin(); it != associatedChannels.at(vid).end(); ++it) {
            if (*it == mapid) {
                found = true;
            }
            break;
        }

        if (found) {
            mapChannel = &(vchan->mapChannels.at(mapid));
        }
        else {
            return;
        }
    }

    // If a segment header exists, the transferFrameDataFieldLength will now represent
    // the actual length of the payload data
    uint8_t securityHeaderOffset = TcPrimaryHeaderSize + vchan->segmentHeaderTCPresent * TcSegmentHeaderSize;
    transferFrameDataFieldLength = transferFrameDataFieldLength - vchan->segmentHeaderTCPresent * TcSegmentHeaderSize;

    uint8_t* frameData = frameTc.getFrameData();
    switch (saConfig) {
        case (HMAC_40_BIT):
            // increase sequence number
            // @TODO handle sequence number overflow (look D3 reference for suggestions)
            sequenceNumber++;

            // insert security parameter index
            frameData[securityHeaderOffset] = static_cast<uint8_t>(securityParameterIndex >> 8);
            frameData[securityHeaderOffset + 1] = static_cast<uint8_t>(securityParameterIndex);

            // insert sequence number
            for (uint8_t i = 0; i < sequenceNumberFieldLength; i++) {
                frameData[securityHeaderOffset + securityParameterIndexLength + initializationVectorLength + i] =
                        static_cast<uint8_t>(sequenceNumber >> ((sequenceNumberFieldLength - i - 1) * 8));
            }

            // apply authentication bitmask to get authentication payload
            uint8_t securityHeaderLength = getSecurityHeaderLength();
            for (uint16_t i = 0; i < securityHeaderOffset + securityHeaderLength + transferFrameDataFieldLength; i++){
                authenticationPayload[i] = frameData[i] & authMaskTC[i];
            }

            // compute MAC
            static tc_hmac_state_struct hmacStruct;
            tc_hmac_set_key(&hmacStruct, authenticationKey, authenticationKeyLength);
            tc_hmac_init(&hmacStruct);
            tc_hmac_update(&hmacStruct, authenticationPayload,
                           securityHeaderOffset + securityHeaderLength + transferFrameDataFieldLength);
            tc_hmac_final(mac, macFieldLength, &hmacStruct);

            // place MAC to the security trailer
            // Note: SHA-256 HMAC outputs a 256 bit long MAC, so it is truncated to macFieldLength*8 bits
            for (uint8_t i = 0; i < macFieldLength; i++) {
                frameData[i + securityHeaderOffset + securityHeaderLength + transferFrameDataFieldLength] = mac[i];
            }

            break;
    // add config specific applySecurity code here
    }
}

SDLSVerificationStatusCode SecurityAssociation::processSecurityTC(TransferFrameTC& frameTc, uint16_t transferFrameDataFieldLength, uint8_t vid, uint8_t mapid) {

    // ensure virtual channel exists and is associated with this SA
    if ((masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) ||
        (associatedChannels.find(vid) == associatedChannels.end())) {
        return INVALID_OR_UNASSOCIATED_CHANNEL;
    }
    VirtualChannel *vchan = &(masterChannel.virtualChannels.at(vid));

    // ensure MAP channel exists (if this virtual channel has map channels) and is associated with this SA
    MAPChannel *mapChannel;
    if (vchan->segmentHeaderTCPresent) {
        if (vchan->mapChannels.find(mapid) == vchan->mapChannels.end()) {
            return INVALID_OR_UNASSOCIATED_CHANNEL;
        }

        bool found = false;
        for (auto it = associatedChannels.at(vid).begin(); it != associatedChannels.at(vid).end(); ++it) {
            if (*it == mapid) {
                found = true;
            }
            break;
        }

        if (found) {
            mapChannel = &(vchan->mapChannels.at(mapid));
        }
        else {
            return INVALID_OR_UNASSOCIATED_CHANNEL;
        }
    }

    // If a segment header exists, the transferFrameDataFieldLength will now represent
    // the actual length of the payload data
    uint8_t securityHeaderOffset = TcPrimaryHeaderSize + vchan->segmentHeaderTCPresent * TcSegmentHeaderSize;
    transferFrameDataFieldLength = transferFrameDataFieldLength - vchan->segmentHeaderTCPresent * TcSegmentHeaderSize;

    uint8_t* frameData = frameTc.getFrameData();
    switch (saConfig) {
        case (HMAC_40_BIT):
            // check security parameter index
            uint16_t receivedSecurityParameterIndex = (static_cast<uint16_t>(frameData[securityHeaderOffset]) << 8) |
                    (static_cast<uint16_t>(frameData[securityHeaderOffset + 1]));
            if (receivedSecurityParameterIndex != securityParameterIndex) {
                return INVALID_SPI;
            }

            // apply authentication bitmask to get authentication payload
            uint8_t securityHeaderLength = getSecurityHeaderLength();
            for (uint16_t i = 0; i < securityHeaderOffset + securityHeaderLength + transferFrameDataFieldLength; i++){
                authenticationPayload[i] = frameData[i] & authMaskTC[i];
            }

            // compute MAC
            static tc_hmac_state_struct hmacStruct;
            tc_hmac_set_key(&hmacStruct, authenticationKey, authenticationKeyLength);
            tc_hmac_init(&hmacStruct);
            tc_hmac_update(&hmacStruct, authenticationPayload,
                           securityHeaderOffset + securityHeaderLength + transferFrameDataFieldLength);
            tc_hmac_final(mac, macFieldLength, &hmacStruct);

            // compare computed MAC with frame's MAC
            bool valid = true;
            for (uint8_t i = 0; i < macFieldLength; i++) {
                if (mac[i] != frameData[i + securityHeaderOffset + securityHeaderLength + transferFrameDataFieldLength]) {
                    valid = false;
                }
            }

            if (!valid) {
                return MAC_VERIFICATION_FAILURE;
            }

            // compare sequence numbers
            uint64_t receivedSeqNumber = 0;
            for (uint8_t i = 0; i < macFieldLength; i++){
                receivedSeqNumber = receivedSeqNumber |
                                    (static_cast<uint64_t>(frameData[securityHeaderOffset + securityParameterIndexLength + initializationVectorLength + macFieldLength -1 -i]) << 8*i);
            }
            if ((receivedSeqNumber <= sequenceNumber) || (receivedSeqNumber > sequenceNumber + sequenceNumberWindow)) {
                return ANTI_REPLAY_SEQUENCE_NUMBER_FAILURE;
            }

            // passed all verification operations: increase sequence number
            sequenceNumber = receivedSeqNumber;

            return NO_FAILURE;
    // add config specific processSecurity code here
    }
}