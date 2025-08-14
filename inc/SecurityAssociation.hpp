/**
 * @file  SecurityAssociation.hpp
 * @brief Functionality related to frame encryption and authentication.
 */

#pragma once
#include <cstdint>
#include "etl/expected.h"
#include "etl/string.h"
#include "CcsdsDefinitions.hpp"
#include "NotificationAndLoggingUtilities/Alert.hpp"
#include "AuthenticationKey.hpp"

namespace CCSDSDataLinkLayer {
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

/**
 * The Security Association (SA) is an entity defined within the Space Data Link Security Protocol
 * (CCSDS 355.0-B-2) and is responsible for offering authentication and encryption capabilities for
 * the Data Link Layer. This specific implementation:
 *  - Is static, meaning that SAs will not be created or destroyed for the duration of the mission
 *  - Stores necessary parameters for both ends. The sending end user is meant to call the 'applySecurity' function,
 *    while the receiving end user is meant to call the 'processSecurity' function. Certain buffers are shared, so
 *    separate instances must be created for the sender and the receiver. The implemented functions support TC frames
 *    (@see TC Space Data Link Protocol), but functions for other CCSDS data link protocols can be implemented in a
 *    similar manner.
 *
 *  @note Support for more service types can be easily extended by adding an extra configuration to the 'Config' enum and then adding specific code
 *  for it in the constructor, applySecurity and process Security functions.
*/
    class SecurityAssociation {
    public:
        SecurityAssociation(const Defs::Spi securityParameterIndex,
                            Defs::AuthenticationAlgorithm authenticationAlgorithm,
                            Defs::EncryptionAlgorithm encryptionAlgorithm)
            : securityParameterIndex(securityParameterIndex),
              authenticationAlgorithm(authenticationAlgorithm),
              encryptionAlgorithm(encryptionAlgorithm) {

            // authentication parameter initialization
            if (authenticationAlgorithm != Defs::AuthenticationAlgorithm::NO_AUTHENTICATION) {
                switch (authenticationAlgorithm) {
                    case (Defs::AuthenticationAlgorithm::HMAC_SHA256_40_BIT):
                        macFieldLength = 5; // 40 bits HMAC

                        authenticationKey = AuthenticationKey;

                        // setup authentication mask (see p.4.2.2.6.2)
                        // mask  virtual channel id, security header, data field
                        authMaskTCLength = Defs::MaxTcTransferFrameLength;
                        authMaskTC[0] = 0x00;
                        authMaskTC[1] = 0x00;
                        authMaskTC[2] = 0xFC;
                        authMaskTC[3] = 0x00;
                        authMaskTC[4] = 0x00;
                        for (uint16_t i = 5; i < Defs::MaxTcTransferFrameLength; i++) {
                            authMaskTC[i] = 0xFF;
                        }

                        sequenceNumberFieldLength = 4;
                        sequenceNumberWindow = 128;
                        break;
                }
            } else {
                // make authentication related fields zero length
                macFieldLength = 0;
                sequenceNumberFieldLength = 0;
            }

            // encryption parameter initialization
            if (encryptionAlgorithm != Defs::EncryptionAlgorithm::NO_ENCRYPTION) {
                // switch (encryptionAlgorithm.value()) {}
            } else {
                // make authentication related fields zero length
                initializationVectorLength = 0;
                padFieldLength = 0;
            }
        }

        [[nodiscard]] uint8_t getSecurityHeaderLength() const;

        [[nodiscard]] uint8_t getSecurityTrailerLength() const;

        [[nodiscard]] Defs::Spi getSecurityParameterIndex() const;

        void resetSequenceNumber();

        /**
         *  @brief Add an authentication code and/or encrypt a transfer frame, for the TC Data Link Protocol (sender side)
         *
         * @details For general sending procedures, @see p. 4.2.3 of the Security Data Link Protocol
         *          For TC Data Link implementation details, @see p. 6.4 of TC Space Data LInk Protocol
         *
         * @param frameTc                       Pointer to transfer frame for the services to be applied. Type_BC frames cannot
         *                                      use security services
         * @param transferFrameDataFieldLength  The length of the payload of the frame AND the segment header
         *
         * @note It is the responsibility of this function's user to insert a frame from a virtual/MAP channel
         *       that is associated with this SA.
         */
        etl::expected<void, SDLSVerificationError>
        applySecurityTC(const TransferFrameTC *frameTc, uint16_t transferFrameDataFieldLength);

        /**
         * @brief Apply security checks to a transfer frame (receiver side)
         *
         * @details For general sending procedures, @see p. 4.2.4 of the Security Data Link Protocol
         *          For TC Data Link implementation details, @see p. 6.5 of TC Space Data LInk Protocol
         *
         * @param frameTc                       Pointer to transfer frame for the services to be applied. Type_BC frames cannot
         *                                      use security services
         * @param transferFrameDataFieldLength  The length of the payload of the frame AND the segment header
         *
         * @note It is the responsibility of this function's user to insert a frame from a virtual/MAP channel
         *       that is associated with this SA.
         */
        etl::expected<void, SDLSVerificationError>
        processSecurityTC(const TransferFrameTC *frameTc, uint16_t transferFrameDataFieldLength);

    private:
        /** ================================================================================
         *   @name General Parameters
         *   For a general overview of the managed parameters and SA management procedures,
         *   @see p. 3.4.2.1 & p. 4.2.2 of Space Data Link Security Protocol
         *  =============================================================================== =
         *  @{
         */

        /**
         * A 2 byte id that is placed in the start of the security header and used to associate a transfer
         * frame with this SA
         * @see p. 4.1.1.2
         */
        Defs::Spi securityParameterIndex;

        /**
         * @}
         */

        /** =========================================================
         *   @name Authentication related parameters
         *   @see table 6-1 from Space Data Link Security Protocol
         *  =========================================================
         *  @{
         */

        /**
         * @note The absence of a value indicates no authentication algorithm will be applied
         */
        Defs::AuthenticationAlgorithm authenticationAlgorithm;

        /**
         * @brief Container for given authentication key.
         */
        [[maybe_unused]] etl::string<Defs::MaxAuthenticationKeyLength> authenticationKey;

        /**
         * @brief Container for holding calculated mac.
         */
        [[maybe_unused]] uint8_t mac[Defs::MaxMACLength];
        uint8_t macFieldLength;

        /**
         * @brief Mask for choosing which fields will be used in the authentication payload.
         */
        [[maybe_unused]] uint8_t authMaskTC[Defs::MaxTcTransferFrameLength];
        [[maybe_unused]] uint16_t authMaskTCLength;

        /**
         *  Sender SA: number that increases in each consecutive frame sent through this SA (replay attack protection)
         *  Receiver SA: accepts frames only if they have a sequence number larger than the stored one
         */
        [[maybe_unused]] uint64_t sequenceNumber = 0;
        uint8_t sequenceNumberFieldLength;

        /**
         *  @brief Largest sequence number accepted by the receiver SA.
         */
        [[maybe_unused]] uint64_t sequenceNumberWindow;

        /**
         * @brief Container for the transfer frame segment that the authentication algorithm will be applied on.
         */
        [[maybe_unused]] uint8_t authenticationPayload[Defs::MaxTcTransferFrameLength];

        /**
         * @}
         */

        /** =========================================================
         *   @name Encryption related parameters
         *   @see table 6-1 from Space Data Link Security Protocol
         *  =========================================================
         *  @{
         */

        /**
         * @note The absence of a value indicates no encryption algorithm will be applied
         */
        Defs::EncryptionAlgorithm encryptionAlgorithm;

        [[maybe_unused]] uint64_t encryptionKey;

        [[maybe_unused]] uint8_t initializationVector[Defs::MaxInitializationVectorLength];
        uint8_t initializationVectorLength;

        [[maybe_unused]] uint16_t padLength;
        uint8_t padFieldLength;

        /**
         * @}
         */
    };
} // namespace CCSDSDataLinkLayer