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
 * NO_SECURITY: Does not offer any security services, security header and trailer lengths are 0 (used for testing purposes).
 * HMAC_40_BIT: AUTHENTICATION service type, using symmetric hash based MACs. SHA-256 is used as the hash function.
 *              They authentication key is 8 bytes in length. To reduce the overhead of MAC inside the frames,
 *              only the 40 leftmost bits are kept.
 *
 */
enum Config {
    NO_SECURITY,
    HMAC_40_BIT
};

enum User {
    SENDER,
    RECEIVER
};

/**
 * The Security Association (SA) is an entity defined within the Space Data Link Security Protocol
 * (CCSDS 355.0-B-2) and is responsible for offering authentication and encryption capabilities for
 * the Data Link Layer. This specific implementation:
 *  - is static, meaning that SAs will not be created and destroyed for the duration of the mission
 *  - Stores necessary parameters for both ends. The sending end user is meant to call the 'applySecurity' function,
 *    while the receiving end user is meant to call the 'processSecurity' function. Certain buffers are shared, so
 *    separate instances MUST be created for the sender and the receiver. The implemented functions support TC frames
 *    (@see TC Space Data Link Protocol), but functions for other CCSDS data link protocols can be implemented in a
 *    similar manner.
 *
 *  Support for more service types can be easily extended by adding an extra configuration to the 'Config' enum and then adding specific code
 *  for it in the constructor, applySecurity and process Security functions.
*/
class SecurityAssociation {
private:
    /**
     * For a general overview of the managed parameters and SA management procedures,
     * @see p. 3.4.2.1 & 0. 4.2.2 of Space Data Link Security Protocol
     */

    /**
     * A 2 byte id that is placed in the start of the security header and used to associate a transfer
     * frame with this SA
     * @see p. 4.1.1.2
     */
    uint16_t securityParameterIndex;

    /**
     * A structure that holds virtual channels associated with this SA, as well their MAP channels
     * @see p. 3.4.2.2.1 & p. 3.4.2.2.2
     */
    etl::flat_map<uint8_t, etl::array<uint8_t, MaxMapChannels>, MaxVirtualChannels> associatedChannels;

    /**
     * Defines which of the 3 services is offered by this SA: authentication, enryption, authenticated encryption
     */
    SecurityAssociationServiceType saServiceType;

    /**
     * A configuration given by the user upon initializing the SA. For a given configuration:
     * 1) Needed authentication parameters are initialized in the constructor (eg. for HMAC_40_BIT, parameters related
     *    to authentication are set up, like the field that holds the MAC being set to 40 bits)
     * 2) Specific code is executed when calling the applySecurity function
     * 3) Specific code is executed when calling the processSecurity function
     *
     * The NO_SECURITY configuration bypasses all security checks (used for testing other aspects of the data link), and
     * is the default one.
     */
    Config saConfig;

    /**
     * SENDER or RECEIVER.The SENDER is not allowed to use the processSecurity function. Similarly, the RECEIVER is not
     * allowed to use the applySecurityFunction.
     */
    User user;

    /**
     * Authentication related parameters.
     */
    AuthenticationAlgorithm authenticationAlgorithm;
    uint8_t authenticationKey[MaxAuthenticationKeyLength]; // container for given authentication key
    uint8_t authenticationKeyLength;

    uint8_t mac[MaxMACLength]; // container for holding calculated mac
    uint8_t macFieldLength;

    uint8_t authMaskTC[MaxTcTransferFrameSize]; // mask for choosing which fields will be used in the authentication payload
    uint16_t authMaskTCLength;

    uint64_t sequenceNumber = 0;         // Sender SA: number that increases in each consecutive frame sent through this SA (replay attack protection)
                                         // Receiver SA: accepts frames only if they have a sequence number larger than the stored one
    uint8_t sequenceNumberFieldLength;
    uint64_t sequenceNumberWindow;      // Largest sequence number accepted by the receiver SA

    uint8_t authenticationPayload[MaxTcTransferFrameSize]; // container for the transfer frame segment that the authentication
                                                           // algorithm will be applied on

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
    SecurityAssociation() {
        saConfig = NO_SECURITY;
    }

    SecurityAssociation(uint16_t securityParameterIndex,
                        etl::flat_map<uint8_t, etl::array<uint8_t, MaxMapChannels>, MaxVirtualChannels>& permittedChannels,
                        Config saConfig, User user) :
            securityParameterIndex(securityParameterIndex), associatedChannels(permittedChannels),
            saConfig(saConfig), user(user) {

        switch (saConfig) {
            case (HMAC_40_BIT):
                saServiceType = AUTHENTICATION;
                authenticationAlgorithm = HMAC;
                encryptionAlgorithm = NO_ENCRYPTION;

                // Make encryption related fields have 0 length
                initializationVectorLength = 0;
                padFieldLength = 0;

                macFieldLength = 5;  // 40 bits HMAC
                authenticationKeyLength = 32;
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
                sequenceNumberWindow = 128;
                break;
            // add config specific initialization code here
        }
    }

    uint8_t getSecurityHeaderLength() const;

    uint8_t getSecurityTrailerLength() const;

    /**
     * Return whether a given virtual channel is associated with this SA or not
     */
    bool isAssociated(uint8_t vid);

    /**
     * Return whether a given map channel is associated with this SA or not
     */
    bool isAssociated(uint8_t vid, uint8_t mapid);

    void resetSequenceNumber();

    /**
     * Add an authentication code and/or encrypt a transfer frame, for the TC Data Link Protocol (sender side)
     * For general sending procedures, @see p. 4.2.3 of the Security Data Link Protocol
     * For TC Data Link implementation details, @see p. 6.4 of TC Space Data LInk Protocol
     * @param frameTc                       Pointer to transfer frame for the services to be applied. Type_BC frames cannot
     *                                      use security services
     * @param transferFrameDataFieldLength  The length of the payload of the frame AND the segment header
     * @param vid                           Virtual channel of the frame
     * @param mapid                         MAP channel of the frame. If MAP channels do not exist for the given virtual channel,
     *                                      this parameter is ignored
     */
    SDLSVerificationStatusCode applySecurityTC(TransferFrameTC* frameTc, uint16_t transferFrameDataFieldLength, uint8_t vid, uint8_t mapid);

    /**
     * Apply security checks to a transfer frame (receiver side)
     * For general sending procedures, @see p. 4.2.4 of the Security Data Link Protocol
     * For TC Data Link implementation details, @see p. 6.5 of TC Space Data LInk Protocol
     * @param frameTc                       Pointer to transfer frame for the services to be applied. Type_BC frames cannot
     *                                      use security services
     * @param transferFrameDataFieldLength  The length of the payload of the frame AND the segment header
     * @param vid                           Virtual channel of the frame
     * @param mapid                         MAP channel of the frame. If MAP channels do not exist for the given virtual channel,
     *                                      this parameter is ignored
     */
    SDLSVerificationStatusCode processSecurityTC(TransferFrameTC* frameTc, uint16_t transferFrameDataFieldLength, uint8_t vid, uint8_t mapid);

};