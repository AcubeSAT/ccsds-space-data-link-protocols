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

/**
 * For cryptographic algorithm recommendations, @see CCSDS CRYPTOGRAPHIC ALGORITHMS (CCSDS 352.0-B-2)
 *
 * Description of configurations
 * =================================================================================================================
 * HMAC_40_BIT: AUTHENTICATION service type, using symmetric hash based MACs. SHA-256 is used as the hash function.
 *              They authentication key is 8 bytes in length. To reduce the overhead of MAC inside the frames,
 *              only the 40 leftmost bits are kept.
 *
 */
enum Config {
    HMAC_40_BIT
};

/**
 * The Security Association (SA) is an entity defined within the Space Data Link Security Protocol
 * (CCSDS 355.0-B-2) and is responsible for offering authentication and encryption capabilities for
 * the Data Link Layer. This specific implementation:
 *  - is static, meaning that SAs will not be created and destroyed for the duration of the mission
 *  - is built as a bidirectional interface. This means that it stores the necessary parameters for both ends.
 *    The sending end user is meant to call the 'applySecurity' function, while the receiving end user is meant to
 *    call the 'processSecurity' function. Certain buffers are shared, so separate instances MUST be created for
 *    the sender and the receiver in testing. The implemented functions support TC frames (@see TC Space Data Link Protocol),
 *    but functions for other CCSDS data link protocols can be implemented in a similar manner.
 *
 *  Support for more service types can be easily extended by adding an extra configuration to the 'Config' enum and then adding specific code
 *  for it in the constructor, applySecurity and process Security functions.
*/
class SecurityAssociation {
private:

    MasterChannel& masterChannel;

    uint16_t securityParameterIndex;

    etl::flat_map<uint8_t, etl::array<uint8_t, MaxMapChannels>, MaxVirtualChannels> associatedChannels;

    SecurityAssociationServiceType saServiceType;

    Config saConfig;

    /**
     * Authentication related parameters. Max lengths (in octets) are defined in table
     * 6-1 (with the exception of the authentication key)
     */
    AuthenticationAlgorithm authenticationAlgorithm;
    uint8_t authenticationKey[MaxAuthenticationKeyLength];
    uint8_t authenticationKeyLength;

    uint8_t mac[MaxMACLength];
    uint8_t macFieldLength;

    uint8_t authMaskTC[MaxTcTransferFrameSize];
    uint16_t authMaskTCLength;

    uint64_t sequenceNumberWindow;
    uint64_t sequenceNumber = 0;
    uint8_t sequenceNumberFieldLength;

    uint8_t authenticationPayload[MaxTcTransferFrameSize];

    /**
     * Encryption related parameters. Max lengths (in octets) are defined in table
     * 6-1 (with the exception of the encryption key)
     */
    EncryptionAlgorithm encryptionAlgorithm;
    uint64_t encryptionKey;

    uint8_t initializationVector[MaxInitializationVectorLength];
    uint8_t initializationVectorLength;

    uint16_t padLength;
    uint8_t padFieldLength;


public:
    SecurityAssociation(MasterChannel& masterChannel,
                        uint16_t securityParameterIndex,
                        etl::flat_map<uint8_t, etl::array<uint8_t, MaxMapChannels>, MaxVirtualChannels>& permittedChannels,
                        Config saConfig) :
            masterChannel(masterChannel) , securityParameterIndex(securityParameterIndex), associatedChannels(permittedChannels) {

        switch (saConfig) {
            case (HMAC_40_BIT):
                saServiceType = AUTHENTICATION;
                authenticationAlgorithm = HMAC;
                encryptionAlgorithm = NO_ENCRYPTION;

                // Make encryption related fields have 0 length
                initializationVectorLength = 0;
                padFieldLength = 0;

                macFieldLength = 8;  // 64 bits HMAC (the minimum defined by table 6-1)
                authenticationKeyLength = 8; // 8 bytes key is the most computationally efficient size with SHA-256
                for (uint8_t i = 0; i < authenticationKeyLength; i++){
                    authenticationKey[i] = AuthenticationKey[i];
                }

                // setup authentication mask (see p.4.2.2.6.2)
                // mask  virtual channel id, security header, data field
                authMaskTCLength = MaxTcTransferFrameSize;
                authMaskTC[0] = 0x00;
                authMaskTC[1] = 0x00;
                authMaskTC[2] = 0xFC;
                authMaskTC[3] = 0x00;
                authMaskTC[4] = 0x00;
                for (uint16_t i = 5; i < MaxTcTransferFrameSize; i++) {
                    authMaskTC[i] = 0xFF;
                }

                sequenceNumberFieldLength = 4;
                sequenceNumberWindow = 100;
                break;
            // add config specific initialization code here
        }
    }

    uint8_t getSecurityHeaderLength() const;

    uint8_t getSecurityTrailerLength() const;

    /**
     * IMPORTANT NOTE: the length of the segment header is included in transferFrameDataFieldLength
     *
     *
     */
    void applySecurityTC(TransferFrameTC& frameTc, uint16_t transferFrameDataFieldLength, uint8_t vid, uint8_t mapid);

    SDLSVerificationStatusCode processSecurityTC(TransferFrameTC& frameTc, uint16_t transferFrameDataFieldLength, uint8_t vid, uint8_t mapid);

};