#include "tinycrypt/hmac.h"
#include "CCSDSSecurityAssociation.hpp"

namespace CCSDSDataLinkLayer {
    uint8_t SecurityAssociation::getSecurityHeaderLength() const {
        if (saConfig == NO_SECURITY) {
            return 0;
        } else {
            return securityParameterIndexLength + initializationVectorLength + sequenceNumberFieldLength +
                   padFieldLength;
        }
    }

    uint8_t SecurityAssociation::getSecurityTrailerLength() const {
        if (saConfig == NO_SECURITY) {
            return 0;
        } else {
            return macFieldLength;
        }
    }

    void SecurityAssociation::resetSequenceNumber() {
        sequenceNumber = 0;
    }

    bool SecurityAssociation::isAssociated(uint8_t vid) {
        if (associatedChannels.find(vid) == associatedChannels.end()) {
            return false;
        } else {
            return true;
        }
    }

    bool SecurityAssociation::isAssociated(uint8_t vid, uint8_t mapid) {
        if (associatedChannels.find(vid) == associatedChannels.end()) {
            return false;
        }

        bool found = false;
        for (auto it = associatedChannels.at(vid).begin(); it != associatedChannels.at(vid).end(); ++it) {
            if (*it == mapid) {
                found = true;
            }
            break;
        }

        if (!found) {
            return false;
        }
        return true;
    }

    SDLSVerificationStatusCode
    SecurityAssociation::applySecurityTC(TransferFrameTC *frameTc, uint16_t transferFrameDataFieldLength, uint8_t vid,
                                         uint8_t mapid) {
        if (saConfig == NO_SECURITY) {
            return SDLSVerificationStatusCode::NO_FAILURE;
        }

        if (user == RECEIVER) {
            return SDLSVerificationStatusCode::INVALID_USER;
        }
        // ensure the frame type is correct (SDLS cannot be applied to type-BC frames)
        if ((frameTc->getServiceType() == ServiceType::TYPE_BC) ||
            (frameTc->getServiceType() == ServiceType::TYPE_RESERVED)) {
            return SDLSVerificationStatusCode::INVALID_FRAME_TYPE;
        }

        // ensure the given channel is associated with this sa
        bool associated = false;
        if (frameTc->getSegmentationHeaderPresentFlag()) {
            associated = isAssociated(vid, mapid);
        } else {
            associated = isAssociated(vid);
        }

        if (!associated) {
            return SDLSVerificationStatusCode::UNASSOCIATED_CHANNEL;
        }

        // If a segment header exists, the transferFrameDataFieldLength will now represent
        // the actual length of the payload data
        uint8_t securityHeaderOffset = TcPrimaryHeaderSize +
                                       frameTc->getSegmentationHeaderPresentFlag() * TcSegmentHeaderSize;
        transferFrameDataFieldLength = transferFrameDataFieldLength -
                                       frameTc->getSegmentationHeaderPresentFlag() * TcSegmentHeaderSize;

        uint8_t *frameData = frameTc->getFrameData();
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
                for (uint16_t i = 0;
                     i < securityHeaderOffset + securityHeaderLength + transferFrameDataFieldLength; i++) {
                    authenticationPayload[i] = frameData[i] & authMaskTC[i];
                }

                // compute MAC
                uint8_t err = 1; // According to tiny crypt documentation, successful operation returns 1
                static tc_hmac_state_struct hmacStruct;
                err = tc_hmac_set_key(&hmacStruct, authenticationKey, authenticationKeyLength);
                if (err != 1) { return SDLSVerificationStatusCode::MAC_CALCULATION_ERROR; }
                err = tc_hmac_init(&hmacStruct);
                if (err != 1) { return SDLSVerificationStatusCode::MAC_CALCULATION_ERROR; }
                err = tc_hmac_update(&hmacStruct, authenticationPayload,
                                     securityHeaderOffset + securityHeaderLength + transferFrameDataFieldLength);
                if (err != 1) { return SDLSVerificationStatusCode::MAC_CALCULATION_ERROR; }
                tc_hmac_final(mac, TC_SHA256_DIGEST_SIZE, &hmacStruct);
                if (err != 1) { return SDLSVerificationStatusCode::MAC_CALCULATION_ERROR; }

                // place MAC to the security trailer
                // Note: SHA-256 HMAC outputs a 256 bit long MAC, so it is truncated to macFieldLength*8 bits
                for (uint8_t i = 0; i < macFieldLength; i++) {
                    frameData[i + securityHeaderOffset + securityHeaderLength + transferFrameDataFieldLength] = mac[i];
                }

                return SDLSVerificationStatusCode::NO_FAILURE;
                break;
                // add config specific applySecurity code here
        }
    }

    SDLSVerificationStatusCode
    SecurityAssociation::processSecurityTC(TransferFrameTC *frameTc, uint16_t transferFrameDataFieldLength, uint8_t vid,
                                           uint8_t mapid) {
        if (saConfig == NO_SECURITY) {
            return SDLSVerificationStatusCode::NO_FAILURE;
        }

        if (user == SENDER) {
            return SDLSVerificationStatusCode::INVALID_USER;
        }


        // ensure the frame type is correct (SDLS cannot be applied to type-BC frames)
        if ((frameTc->getServiceType() == ServiceType::TYPE_BC) ||
            (frameTc->getServiceType() == ServiceType::TYPE_RESERVED)) {
            return SDLSVerificationStatusCode::INVALID_FRAME_TYPE;
        }

        // ensure the given channel is associated with this sa
        bool associated = false;
        if (frameTc->getSegmentationHeaderPresentFlag()) {
            associated = isAssociated(vid, mapid);
        } else {
            associated = isAssociated(vid);
        }

        if (!associated) {
            return SDLSVerificationStatusCode::UNASSOCIATED_CHANNEL;
        }

        // If a segment header exists, the transferFrameDataFieldLength will now represent
        // the actual length of the payload data
        uint8_t securityHeaderOffset = TcPrimaryHeaderSize +
                                       frameTc->getSegmentationHeaderPresentFlag() * TcSegmentHeaderSize;
        transferFrameDataFieldLength = transferFrameDataFieldLength -
                                       frameTc->getSegmentationHeaderPresentFlag() * TcSegmentHeaderSize;

        uint8_t *frameData = frameTc->getFrameData();
        switch (saConfig) {
            case (HMAC_40_BIT):
                // check security parameter index
                uint16_t receivedSecurityParameterIndex =
                        (static_cast<uint16_t>(frameData[securityHeaderOffset]) << 8) |
                        (static_cast<uint16_t>(frameData[securityHeaderOffset + 1]));
                if (receivedSecurityParameterIndex != securityParameterIndex) {
                    return SDLSVerificationStatusCode::INVALID_SPI;
                }

                // apply authentication bitmask to get authentication payload
                uint8_t securityHeaderLength = getSecurityHeaderLength();
                for (uint16_t i = 0;
                     i < securityHeaderOffset + securityHeaderLength + transferFrameDataFieldLength; i++) {
                    authenticationPayload[i] = frameData[i] & authMaskTC[i];
                }

                // compute MAC
                uint8_t err = 1; // According to tiny crypt documentation, successful operation returns 1
                static tc_hmac_state_struct hmacStruct;
                tc_hmac_set_key(&hmacStruct, authenticationKey, authenticationKeyLength);
                if (err != 1) { return SDLSVerificationStatusCode::MAC_CALCULATION_ERROR; }
                tc_hmac_init(&hmacStruct);
                if (err != 1) { return SDLSVerificationStatusCode::MAC_CALCULATION_ERROR; }
                tc_hmac_update(&hmacStruct, authenticationPayload,
                               securityHeaderOffset + securityHeaderLength + transferFrameDataFieldLength);
                if (err != 1) { return SDLSVerificationStatusCode::MAC_CALCULATION_ERROR; }
                tc_hmac_final(mac, TC_SHA256_DIGEST_SIZE, &hmacStruct);
                if (err != 1) { return SDLSVerificationStatusCode::MAC_CALCULATION_ERROR; }

                // compare computed MAC with frame's MAC
                bool valid = true;
                for (uint8_t i = 0; i < macFieldLength; i++) {
                    if (mac[i] !=
                        frameData[i + securityHeaderOffset + securityHeaderLength + transferFrameDataFieldLength]) {
                        valid = false;
                    }
                }

                if (!valid) {
                    return SDLSVerificationStatusCode::MAC_VERIFICATION_FAILURE;
                }

                // compare sequence numbers
                uint64_t receivedSeqNumber = 0;
                for (uint8_t i = 0; i < sequenceNumberFieldLength; i++) {
                    receivedSeqNumber = receivedSeqNumber |
                                        (static_cast<uint64_t>(frameData[securityHeaderOffset +
                                                                         securityParameterIndexLength +
                                                                         initializationVectorLength +
                                                                         sequenceNumberFieldLength - 1 - i]) << 8 * i);
                }
                if ((receivedSeqNumber <= sequenceNumber) ||
                    (receivedSeqNumber > sequenceNumber + sequenceNumberWindow)) {
                    return SDLSVerificationStatusCode::ANTI_REPLAY_SEQUENCE_NUMBER_FAILURE;
                }

                // passed all verification operations: update sequence number
                sequenceNumber = receivedSeqNumber;

                return SDLSVerificationStatusCode::NO_FAILURE;
                break;
                // add config specific processSecurity code here
        }
    }
} // namespace CCSDSDataLinkLayer