/**
 * @file  SecurityAssociation.hpp
 * @brief Functionality related to frame encryption and authentication.
 */

#pragma once
#include <cstdint>
#include "etl/expected.h"
#include "etl/string.h"
#include "CcsdsDefinitions.hpp"
#include "DataLinkNotifications.hpp"
#include "Mutex.hpp"
#include "TransferFrameTC.hpp"
#include "TransferFrameTM.hpp"

namespace CCSDSDataLinkLayer {
    enum class SecurityAssociationStatus: bool {
        PAUSED,
        RUNNING
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
            : saStatus(SecurityAssociationStatus::RUNNING),
              securityParameterIndex(securityParameterIndex),
              authenticationAlgorithm(authenticationAlgorithm),
              encryptionAlgorithm(encryptionAlgorithm) {

            // authentication parameter initialization
            if (authenticationAlgorithm != Defs::AuthenticationAlgorithm::NO_AUTHENTICATION) {
                switch (authenticationAlgorithm) {
                    case (Defs::AuthenticationAlgorithm::HMAC_SHA256_40_BIT):
                        initializationVectorLength = 0;
                        sequenceNumberFieldLength = 4;
                        padFieldLength = 0;
                        macFieldLength = 5; // 40 bits HMAC
                        sequenceNumberWindow = 128;
                        break;
                }
            } else {
                // make authentication related fields zero length
                sequenceNumberFieldLength = 0;
                macFieldLength = 0;
            }

            // setup TC authentication mask (see p.4.2.2.6.2)
            // mask  virtual channel id, security header, segment header, data field
            authMaskTC[0] = 0x00;
            authMaskTC[1] = 0x00;
            authMaskTC[2] = 0xFC; // virtual channel id
            authMaskTC[3] = 0x00;
            authMaskTC[4] = 0x00;
            authMaskTC[5] = 0xFF; // segment header
            authMaskTC[6] = 0xFF; // first SPI octet
            authMaskTC[7] = 0xFF; // second SPI octet
            for (uint16_t i = 8 + initializationVectorLength; i < Defs::MaxTcTransferFrameLength; i++) {
                // Rest of security header (except initialization vector field) and data field
                authMaskTC[i] = 0xFF;
            }

            // setup TM authentication mask (see p.4.2.2.6.2)
            // mask  virtual channel id, security header, secondary header, data field
            authMaskTM[0] = 0x00;
            authMaskTM[1] = 0x0E; // virtual channel id
            authMaskTM[2] = 0x00;
            authMaskTM[3] = 0x00;
            authMaskTM[4] = 0x00;
            authMaskTM[5] = 0x00;
            for (uint16_t i = 6; i < Defs::MaxTcTransferFrameLength; i++) {
                // Secondary header, security header and data field
                // Note: Unfortunately we cannot have 0x00 in the initialization vector position, due
                //       to the fact that the secondary header is variable length.
                //       TODO If encryption is ever implemented, handle the above dynamically, inside the security functions
                authMaskTM[i] = 0xFF;
            }
        }

        [[nodiscard]] SecurityAssociationStatus getSecurityAssociationStatus() const {
            return saStatus;
        }

        void setSecurityAssociationStatus(SecurityAssociationStatus status) {
            saStatus = status;
        }

        [[nodiscard]] uint8_t getSecurityHeaderLength() const;

        [[nodiscard]] uint8_t getSecurityTrailerLength() const;

        [[nodiscard]] Defs::Spi getSecurityParameterIndex() const;

        void resetSequenceNumber();

        /**
         *  @brief Add an authentication code and/or encrypt a transfer frame (sender side)
         *
         * @details For general sending procedures, @see p. 4.2.3 of the Security Data Link Protocol
         *          For TC Data Link implementation details, @see p. 6.4 of TC Space Data LInk Protocol
         *          For TM Data Link implementation details, @see p. 6.4 of TM Space Data LInk Protocol
         *
         * @param frame  Pointer to transfer frame for the services to be applied. Note that Type_BC frames cannot
         *               use security services
         * @param transferFrameDataFieldLength  In case of TC frame, this length includes the segment header as well
         *
         * @note It is the responsibility of this function's user to insert a frame from a virtual/MAP channel
         *       that is associated with this SA.
         */
        etl::expected<void, SDLSVerificationError>
        applySecurity(etl::variant<TransferFrameTC*, TransferFrameTM*> frame, uint16_t transferFrameDataFieldLength);

        /**
         * @brief Apply security checks to a transfer frame (receiver side)
         *
         * @details For general sending procedures, @see p. 4.2.4 of the Security Data Link Protocol
         *          For TC Data Link implementation details, @see p. 6.5 of TC Space Data LInk Protocol
         *          For TM Data Link implementation details, @see p. 6.5 of TM Space Data LInk Protocol
         *
         * @param frame  Pointer to transfer frame for the services to be applied. Note that Type_BC frames cannot
         *               use security services
         * @param transferFrameDataFieldLength  In case of TC frame, this length includes the segment header as well
         *
         * @note It is the responsibility of this function's user to insert a frame from a virtual/MAP channel
         *       that is associated with this SA.
         */
        etl::expected<void, SDLSVerificationError>
        processSecurity(etl::variant<TransferFrameTC*, TransferFrameTM*> frame, uint16_t transferFrameDataFieldLength);

        /**
         * @brief Prevent concurrent usage by Virtual/MAP Channels that share the same sa
         */
        Mutex saMutex;

    private:
        /**
         * @brief Used to indicate if SA is currently active or not. A paused SA returns an error if the security
         *        application/processing functions are called.
         */
        SecurityAssociationStatus saStatus;

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
         * @brief Container for holding calculated mac.
         */
        [[maybe_unused]] uint8_t mac[Defs::MaxMACLength];
        uint8_t macFieldLength;

        /**
         * @brief Masks for choosing which fields will be used in the authentication payload.
         */
        [[maybe_unused]] static constexpr uint16_t authMaskTCLength = Defs::MaxTcTransferFrameLength;
        [[maybe_unused]] uint8_t authMaskTC[authMaskTCLength];

        [[maybe_unused]] static constexpr uint16_t authMaskTMLength = Defs::MaxTmTransferFrameLength;
        [[maybe_unused]] uint8_t authMaskTM[authMaskTMLength];

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

        [[maybe_unused]] uint8_t initializationVector[Defs::MaxInitializationVectorLength];
        uint8_t initializationVectorLength;

        [[maybe_unused]] uint16_t padLength;
        uint8_t padFieldLength;

        /**
         * @}
         */
    };
} // namespace CCSDSDataLinkLayer