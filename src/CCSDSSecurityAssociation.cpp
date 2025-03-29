#include "CCSDSSecurityAssociation.hpp"
#include "tinycrypt/hmac.h"
#include "CCSDSChannel.hpp"

namespace CCSDSDataLinkLayer {
    uint8_t SecurityAssociation::getSecurityHeaderLength() const {
        return DefsAndUtils::securityParameterIndexLength + initializationVectorLength + sequenceNumberFieldLength +
               padFieldLength;
    }

    uint8_t SecurityAssociation::getSecurityTrailerLength() const {
        return macFieldLength;
    }

    uint16_t SecurityAssociation::getSecurityParameterIndex() const {
        return securityParameterIndex;
    }

    void SecurityAssociation::resetSequenceNumber() {
        sequenceNumber = 0;
    }

    etl::expected<void, SDLSVerificationError>
    SecurityAssociation::applySecurityTC(const TransferFrameTC *frameTc, uint16_t transferFrameDataFieldLength) {
        if (!authenticationAlgorithm.has_value() && !encryptionAlgorithm.has_value()) {
            return etl::unexpected(SDLSVerificationError::NO_SECURITY_CONFIGURED);
        }

        if (user == DefsAndUtils::SecurityAssociationUser::RECEIVER) {
            return etl::unexpected(SDLSVerificationError::INVALID_USER);
        }

        // ensure the frame type is correct (SDLS cannot be applied to type-BC frames)
        if ((frameTc->getServiceType() == DefsAndUtils::ServiceType::TYPE_BC) ||
            (frameTc->getServiceType() == DefsAndUtils::ServiceType::TYPE_RESERVED)) {
            return etl::unexpected(SDLSVerificationError::INVALID_FRAME_TYPE);
        }

        // If a segment header exists, the transferFrameDataFieldLength will now represent
        // the actual length of the payload data
        uint8_t securityHeaderOffset = DefsAndUtils::TcPrimaryHeaderSize +
                                       frameTc->getSegmentationHeaderPresentFlag() * DefsAndUtils::TcSegmentHeaderSize;
        transferFrameDataFieldLength = transferFrameDataFieldLength -
                                       frameTc->getSegmentationHeaderPresentFlag() * DefsAndUtils::TcSegmentHeaderSize;

        uint8_t *frameData = frameTc->getFrameData();

        // Authentication processing
        if (authenticationAlgorithm.has_value()) {
            switch (authenticationAlgorithm.value()) {
                case (DefsAndUtils::AuthenticationAlgorithm::HMAC_SHA256_40_BIT):
                    // increase sequence number
                    // @TODO handle sequence number overflow (look D3 reference for suggestions)
                    sequenceNumber++;

                // insert security parameter index
                    frameData[securityHeaderOffset] = static_cast<uint8_t>(securityParameterIndex >> 8);
                    frameData[securityHeaderOffset + 1] = static_cast<uint8_t>(securityParameterIndex);

                // insert sequence number
                    for (uint8_t i = 0; i < sequenceNumberFieldLength; i++) {
                        frameData[securityHeaderOffset + DefsAndUtils::securityParameterIndexLength +
                                  initializationVectorLength + i]
                                =
                                static_cast<uint8_t>(sequenceNumber >> ((sequenceNumberFieldLength - i - 1) * 8));
                    }

                // apply authentication bitmask to get authentication payload
                    const uint8_t securityHeaderLength = getSecurityHeaderLength();
                    for (uint16_t i = 0;
                         i < securityHeaderOffset + securityHeaderLength + transferFrameDataFieldLength; i++) {
                        authenticationPayload[i] = frameData[i] & authMaskTC[i];
                    }

                // compute MAC
                    uint8_t err; // According to tiny crypt documentation, successful operation returns 1
                    static tc_hmac_state_struct hmacStruct;
                    err = tc_hmac_set_key(&hmacStruct, reinterpret_cast<uint8_t *>(authenticationKey.data()),
                                          authenticationKey.size());
                    if (err != 1) { return etl::unexpected(SDLSVerificationError::MAC_CALCULATION_ERROR); }
                    err = tc_hmac_init(&hmacStruct);
                    if (err != 1) { return etl::unexpected(SDLSVerificationError::MAC_CALCULATION_ERROR); }
                    err = tc_hmac_update(&hmacStruct, authenticationPayload,
                                         securityHeaderOffset + securityHeaderLength + transferFrameDataFieldLength);
                    if (err != 1) { return etl::unexpected(SDLSVerificationError::MAC_CALCULATION_ERROR); }
                    err = tc_hmac_final(mac, TC_SHA256_DIGEST_SIZE, &hmacStruct);
                    if (err != 1) { return etl::unexpected(SDLSVerificationError::MAC_CALCULATION_ERROR); }

                // place MAC to the security trailer
                // Note: SHA-256 HMAC outputs a 256 bit long MAC, so it is truncated to macFieldLength*8 bits
                    for (uint8_t i = 0; i < macFieldLength; i++) {
                        frameData[i + securityHeaderOffset + securityHeaderLength + transferFrameDataFieldLength] = mac[
                            i];
                    }
                    break;
            }
        }

        // // Encryption Processing
        // if (encryptionAlgorithm.has_value()) {
        //
        // }

        return {};
    }

    etl::expected<void, SDLSVerificationError>
    SecurityAssociation::processSecurityTC(const TransferFrameTC *frameTc, uint16_t transferFrameDataFieldLength) {
        if (!authenticationAlgorithm.has_value() && !encryptionAlgorithm.has_value()) {
            return etl::unexpected(SDLSVerificationError::NO_SECURITY_CONFIGURED);
        }

        if (user == DefsAndUtils::SecurityAssociationUser::SENDER) {
            return etl::unexpected(SDLSVerificationError::INVALID_USER);
        }

        // ensure the frame type is correct (SDLS cannot be applied to type-BC frames)
        if ((frameTc->getServiceType() == DefsAndUtils::ServiceType::TYPE_BC) ||
            (frameTc->getServiceType() == DefsAndUtils::ServiceType::TYPE_RESERVED)) {
            return etl::unexpected(SDLSVerificationError::INVALID_FRAME_TYPE);
        }

        // If a segment header exists, the transferFrameDataFieldLength will now represent
        // the actual length of the payload data
        const uint8_t securityHeaderOffset = DefsAndUtils::TcPrimaryHeaderSize +
                                             frameTc->getSegmentationHeaderPresentFlag() *
                                             DefsAndUtils::TcSegmentHeaderSize;
        transferFrameDataFieldLength = transferFrameDataFieldLength -
                                       frameTc->getSegmentationHeaderPresentFlag() * DefsAndUtils::TcSegmentHeaderSize;

        const uint8_t *frameData = frameTc->getFrameData();

        // Authentication processing
        if (authenticationAlgorithm.has_value()) {
            switch (authenticationAlgorithm.value()) {
                case (DefsAndUtils::AuthenticationAlgorithm::HMAC_SHA256_40_BIT):
                    // check security parameter index
                    const uint16_t receivedSecurityParameterIndex =
                            (static_cast<uint16_t>(frameData[securityHeaderOffset]) << 8) |
                            (static_cast<uint16_t>(frameData[securityHeaderOffset + 1]));
                    if (receivedSecurityParameterIndex != securityParameterIndex) {
                        return etl::unexpected(SDLSVerificationError::INVALID_SPI);
                    }

                // apply authentication bitmask to get authentication payload
                    const uint8_t securityHeaderLength = getSecurityHeaderLength();
                    for (uint16_t i = 0;
                         i < securityHeaderOffset + securityHeaderLength + transferFrameDataFieldLength; i++) {
                        authenticationPayload[i] = frameData[i] & authMaskTC[i];
                    }

                // compute MAC
                    uint8_t err; // According to tiny crypt documentation, successful operation returns 1
                    static tc_hmac_state_struct hmacStruct;
                    err = tc_hmac_set_key(&hmacStruct, reinterpret_cast<uint8_t *>(authenticationKey.data()),
                                          authenticationKey.size());;
                    if (err != 1) { return etl::unexpected(SDLSVerificationError::MAC_CALCULATION_ERROR); }
                    err = tc_hmac_init(&hmacStruct);
                    if (err != 1) { return etl::unexpected(SDLSVerificationError::MAC_CALCULATION_ERROR); }
                    err = tc_hmac_update(&hmacStruct, authenticationPayload,
                                         securityHeaderOffset + securityHeaderLength + transferFrameDataFieldLength);
                    if (err != 1) { return etl::unexpected(SDLSVerificationError::MAC_CALCULATION_ERROR); }
                    err = tc_hmac_final(mac, TC_SHA256_DIGEST_SIZE, &hmacStruct);
                    if (err != 1) { return etl::unexpected(SDLSVerificationError::MAC_CALCULATION_ERROR); }

                // compare computed MAC with frame's MAC
                    bool valid = true;
                    for (uint8_t i = 0; i < macFieldLength; i++) {
                        if (mac[i] !=
                            frameData[i + securityHeaderOffset + securityHeaderLength + transferFrameDataFieldLength]) {
                            valid = false;
                            break;
                        }
                    }

                    if (!valid) {
                        return etl::unexpected(SDLSVerificationError::MAC_VERIFICATION_FAILURE);
                    }

                // compare sequence numbers
                    uint64_t receivedSeqNumber = 0;
                    for (uint8_t i = 0; i < sequenceNumberFieldLength; i++) {
                        receivedSeqNumber = receivedSeqNumber |
                                            (static_cast<uint64_t>(frameData[securityHeaderOffset +
                                                                             DefsAndUtils::securityParameterIndexLength
                                                                             +
                                                                             initializationVectorLength +
                                                                             sequenceNumberFieldLength - 1 - i]) << 8 *
                                             i);
                    }
                    if ((receivedSeqNumber <= sequenceNumber) ||
                        (receivedSeqNumber > sequenceNumber + sequenceNumberWindow)) {
                        return etl::unexpected(SDLSVerificationError::ANTI_REPLAY_SEQUENCE_NUMBER_FAILURE);
                    }

                // passed all verification operations: update sequence number
                    sequenceNumber = receivedSeqNumber;
            }
        }

        // Encryption processing
        // if (encryptionAlgorithm.has_value()) {
        //
        // }

        return {};
    }
} // namespace CCSDSDataLinkLayer
