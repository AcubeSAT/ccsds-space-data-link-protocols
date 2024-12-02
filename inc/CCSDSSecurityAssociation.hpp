#pragma once
#include <cstdint>
#include <CCSDS_Definitions.hpp>
#include <etl/flat_map.h>
#include <etl/array.h>
#include <optional>
#include "TransferFrame.hpp"
#include "TransferFrameTC.hpp"
#include "TransferFrameTM.hpp"
#include "Alert.hpp"
#include "CCSDSChannel.hpp"
#include "CCSDSLoggerImpl.h"

enum SecurityAssociationServiceType {
    AUTHENTICATION,
    ENCRYPTION,
    AUTHENTICATED_ENCRYPTION
};

enum AuthenticationAlgorithm {
    HMAC,
    NO_AUTHENTICATION
};

enum EncryptionAlgorithm {
    NO_ENCRYPTION
};

enum Config {
    HMAC_40_BIT
};

/**
 * The Security Association (SA) is an entity defined within the Space Data Link Security Protocol
 * (CCSDS 355.0-B-2) and is responsible for offering authentication and encryption capabilities for
 * the Data Link Layer. This specific implementation:
 *  - is static, meaning that SAs will not be created and destroyed for the duration of the mission
 *  - offers only authentication capabilities (40-bit HMAC)
 *  - supports only the TC Data Link Protocol
 *  - is built as a bidirectional interface. This means that it stores the necessary parameters for both ends.
 *    the sending end user is meant to call the 'applySecurity' functions, while the receiving end user is meant to
 *    call the 'processSecurity' functions.
 *
 *  Support for more features can be easily extended by using an extra Config and adding Config specific code
*/
class SecurityAssociation {
private:

    MasterChannel& masterChannel;

    uint16_t securityParameterIndex;

    etl::flat_map<uint8_t, etl::array<uint8_t, MaxMapChannels>, MaxVirtualChannels> associatedChannels;

    SecurityAssociationServiceType saServiceType;

    Config saConfig;

    /**
     * Authentication related parameters
     */
    AuthenticationAlgorithm authenticationAlgorithm;
    uint16_t authKey;
    uint8_t macLength;
    uint8_t authMaskTC[MaxTcTransferFrameSize];
    uint16_t authMaskTCLength;
    uint8_t sequenceNumberLength;
    uint64_t sequenceNumberWindow;
    uint64_t senderSequenceNumber;
    uint64_t receiverSequenceNumber;

    /**
     * Encryption related parameters
     */
    EncryptionAlgorithm encryptionAlgorithm;
    uint16_t encryptionKey;
    uint8_t initializationVectorLength;
    uint8_t padLength;


public:
    SecurityAssociation(MasterChannel& masterChannel,
                        uint16_t securityParameterIndex,
                        etl::flat_map<uint8_t, etl::array<uint8_t, MaxMapChannels>, MaxVirtualChannels>& permittedChannels,
                        Config saConfig) :

            masterChannel(masterChannel) , securityParameterIndex(securityParameterIndex), associatedChannels(permittedChannels){


        switch (saConfig) {
            case (HMAC_40_BIT):
                saServiceType = AUTHENTICATION;
                authenticationAlgorithm = HMAC;
                encryptionAlgorithm = NO_ENCRYPTION;

                initializationVectorLength = 0;
                padLength = 0;
                break;
            // add config specific initialization code here
        }
    }

    void applySecurityTC(TransferFrameTC& frameTc, uint8_t vid, uint8_t mapid);

    SDLSVerificationStatusCode processSecurityTC(TransferFrameTC& frameTc, uint8_t vid, uint8_t mapid);

};