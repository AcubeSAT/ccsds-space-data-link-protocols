/**
 * @file CCSDSDefinitionsAndUtilities.hpp
 * @brief All definitions used throughout the Data Link.
 */

#pragma once

#include <cstdint>
#include "etl/vector.h"
#include "etl/span.h"
#include "ExternalContainers.hpp"

namespace CCSDSDataLinkLayer {
    class TransferFrameTC;
}

namespace CCSDSDataLinkLayer::Defs {
    /** =============================
     *   @name Tc and Tm frame enums
     *  =============================
     *  @{
     */

    /**
     * @brief Defines the type of protocol used
     * @see SANA transfer frame version numbers registry
     */
    enum class TransferFrameVersionNumber: uint8_t {
        UNASSIGNED = 0b11,
        AOS_SYNCHRONOUS_TRANSFER_FRAME_V2 = 0b01,
        PROXIMITY_1_TRANSFER_FRAME_V3 = 0b10,
        TM_TC_SYNCHRONOUS_TRANSFER_FRAME_V1 = 0b00,
        USLP_TRANSFER_FRAME_V4 = 0b1100
    };

    /**
     * @brief Indicates whether a frame is of type TC (telecommand) or TM (telemetry)
     */
    enum class FrameType : bool {
        TC = false,
        TM = true
    };

    /**
     * @brief Indicates the type of data carried by the TM transfer frame.
     *
     * @details OCTET_SYNCHRONIZED_FORWARD_ORDERED packets are of known structure to the data link ("Space Packets"
     * or "Encapsulation Packets"). VCA_SDU are service data units of unknown structure.
     *
     * @see p. 4.1.2.7.3 from TM Space Data Link Protocol
     */
    enum class SynchronizationFlag : bool {
        OCTET_SYNCHRONIZED_FORWARD_ORDERED = false,
        VCA_SDU = true
    };

    /**
     * @brief Indicates the type of data carried by the TC transfer frame.
     *
     * @details PACKETS are of known structure to the data link ("Space Packets"
     * or "Encapsulation Packets"). VCA_SDU are service data units of unknown structure.
     *
     * @see tables 5-3, 5-4 from TC Space Data Link Protocol
     *
     * @note Those definitions are the same as those of the TM Data Link Protocol. The duplicate
     *       enum exists for the sake of keeping the correct terminology in each protocol
     */
    enum class DataFieldContent : bool {
        PACKET = false,
        VCA_SDU = true
    };

    /**
     *  @brief A TC frame's service type.
     *  @see p. 2.2.2 from TC SPACE DATA LINK PROTOCOL
     */
    enum class ServiceType : uint8_t {
        TYPE_AD = 0x0,
        TYPE_RESERVED = 0x1,
        TYPE_BD = 0x2,
        TYPE_BC = 0x3,
    };

    /**
     * @brief Indicates whether a TC frame carries a segmented packet.
     */
    enum class SequenceFlag : uint8_t {
        SEGMENTATION_MIDDLE = 0x0,
        SEGMENTATION_START = 0x1,
        SEGMENTATION_END = 0x2,
        NO_SEGMENTATION = 0x3
    };

    /**
     * @brief The version of the TM frame secondary header. Currently, only one version is defined
     */
    enum class SecondaryHeaderVersionNumber : uint8_t {
        VERSION_1 = 0x0,
        RESERVED_1 = 0x1,
        RESERVED_2 = 0x2,
        RESERVED_3 = 0x3,
    };

    /**
     * @}
     */

    /** ===============================================
     *    @name Channel Addressing types
     *  ===============================================
     *  @{
     */
    using Pcid = uint8_t; // not defined by ccsds standards, used for easier addressing

    using Mapid = uint8_t; // actual size is 6 bits

    using Vcid = uint8_t; // actual size is 6 bits

    using Scid = uint16_t; // actual size is 10 bits

    // These types will be used as keys in maps, so that every virtual and map channel can be uniquely identified
    // Master channels can be uniquely identified by their scid, since by design, they are unique (CCSDS assigns scids)

    /**
     *  | VCID (6 bits) | SCID(10 bits) |
     */
    using VcidScidKey = uint16_t;

    /**
     *  | EMPTY (10 bits) | MAPID (6 bits) | VCID (6 bits) | SCID (10 bits) |
     */
    using MapidVcidScidKey = uint32_t;

    /**
     * @}
     */

    /** =======================================
     *   @name Field lengths - Field constants
     *  =======================================
     *  @{
     */

    inline constexpr uint8_t ErrorControlFieldSize = 2;

    inline constexpr uint8_t TmPrimaryHeaderSize = 6;
    inline constexpr uint8_t TmSecondaryHeaderIdLength = 1;
    inline constexpr uint8_t TmOperationalControlFieldSize = 4;
    inline constexpr uint16_t TmOIDFrameFirstHeaderPointer = 0x7FE;
    inline constexpr uint16_t TmNoPacketStartFirstHeaderPointerVal = 0x7FF;

    inline constexpr uint8_t TcPrimaryHeaderSize = 5;
    inline constexpr uint8_t TcSegmentHeaderSize = 1;

    /**
     * If the virtual channel packet service is used, the packet Order field is reserved for future use. Setting the value to
     * zero guarantees conformance.
     *
     * If the virtual channel access service is defined this flag is user defined (its use is optional).
     *
     * @see p. 4.1.2.7.4 from TM SPACE DATA LINK PROTOCOL
     * @see p. 3.4.2.3  from TM SPACE DATA LINK PROTOCOL
     */
    inline constexpr bool PacketOrderFlag = 0;

    /**
     * If the virtual channel packet service is used, the segment length field is unused (legacy field). Setting the value to
     * zero guarantees conformance.
     *
     * If the virtual channel access service is defined this flag is user defined (its use is optional).
     *
     * @see p. 4.1.2.7.5 from TM SPACE DATA LINK PROTOCOL
     * @see p. 3.4.2.3  from TM SPACE DATA LINK PROTOCOL
     */
    inline constexpr uint8_t SegmentLengthIdentifierLegacy = 3;

    /**
     *  Maximum allowed TC transfer frame length.
     *  @see p. 5.2 from TC SPACE DATA LINK PROTOCOL
     */
    inline constexpr uint16_t MaxTcTransferFrameLength = 128;

    /**
     * Maximum TM transfer frame length.
     */
    inline constexpr uint16_t MaxTmTransferFrameLength = 128;

    /**
     * How many secondary header fields an ss tm virtual channel can store
     */
    inline constexpr uint16_t SecondaryHeaderFieldCapacity = 5;

    /**
     * @}
     */

    /** ============================================
     *   @name Packet constants
     *   @see CCSDS Space Packet Protocol
     *   @see CCSDS Encapsulation Packet Protocol
     *  ============================================
     *   @{
     */

    /**
     * @brief Packets allowed by CCSDS at the time of writing
     * @see SANA Packet Version Number Registry for an up-to-date list
     */
    enum class PacketVersionNumber : uint8_t {
        SPACE_PACKET = 0x00,
        ENCAPSULATION_PACKET = 0x07,
    };

    /**
     * General space packet constants
     */
    inline constexpr uint8_t SpacePacketPrimaryHeaderLength = 6;
    inline constexpr uint8_t SpacePacketDataLengthFieldPosition = 5; // 5th and 6th bytes constitute the data field length

    /**
     * Idle space packet constants
     */
    inline constexpr bool IdleSpPacketType = false;          // 0/false is for telemetry
    inline constexpr bool IdleSpSecondaryHeaderFlag = false; // Must always be false for idle packets
    inline constexpr uint16_t IdleSpAPID = 0x7FF;            // Reserved value for idle packets
    inline constexpr uint8_t IdleSPUnsegmentedDataSeqFlag = 0x3; // Unsegmented data
    inline constexpr uint16_t IdleSpSequenceCount = 0x0;     // The receiver will not pass idle packets to the upper layer, so this value does not need to be managed
    inline constexpr uint8_t IdleSpPrimaryHeader[SpacePacketPrimaryHeaderLength] = {
        (static_cast<uint8_t>(PacketVersionNumber::SPACE_PACKET) << 5) | (IdleSpPacketType << 4) | (
            IdleSpSecondaryHeaderFlag << 3) | (IdleSpAPID >> 8),
        static_cast<uint8_t>(IdleSpAPID),
        (IdleSPUnsegmentedDataSeqFlag << 6) | (IdleSpSequenceCount >> 8),
        static_cast<uint8_t>(IdleSpSequenceCount)
    };

    /**
     * @brief Encapsulation packets have multiple optional fields, making the packet header have a variable length.
     *        The user should specify whether:
     *        - The "User Defined" and "Encapsulation protocol ID extension fields exist" (1 octet)
     *        - The "CCSDS Defined" field exists (2 octets)
     *
     * @see figure 4-2 and table 4-2 of ENCAPSULATION PACKET PROTOCOL CCSDS
     */

    inline constexpr bool EpUserDefinedAndExtentionFieldsPresent = true;
    inline constexpr bool EpCcsdsFieldPresent = true;

    inline constexpr uint8_t EpMandatoryFieldsLength = 1; // all lengths refer to octets
    inline constexpr uint8_t EpCcsdsFieldLength = 2;
    inline constexpr uint8_t EpUserDefinedAndExtentionFieldsLength = 1;
    inline constexpr uint8_t MaximumEppPacketLengthFieldSize = 4;

    inline constexpr uint8_t EpTotalOffet =
        EpMandatoryFieldsLength +
        EpUserDefinedAndExtentionFieldsPresent * MaximumEppPacketLengthFieldSize +
        EpCcsdsFieldPresent * EpCcsdsFieldLength;

    /**
     * @}
     */

    /** ===================================
     *   @name COP definitions
     *  ===================================
     *  @{
     */

    inline constexpr uint8_t DirectiveRequestSignalQueueSize = 5;
    inline constexpr uint8_t ClcwQueueSize = 1; // Size of 1, since only the most recent state of FARM-1 is of interest
    inline constexpr uint8_t DirectiveNotificationSignalQueueSize = 5;
    inline constexpr uint8_t AsynchronousNotificationSignalQueueSize = 3;
    inline constexpr uint8_t FduTransferSignalQueueSize = 10;
    inline constexpr uint8_t LowerLayerResponseSignalQueueSize = 10;
    inline constexpr uint8_t TransferNotificationSignalQueueSize = 10;
    inline constexpr uint8_t FopToLowerLayerRequestSignalQueueSize = 10;
    inline constexpr uint8_t SentQueueSize = 10;

    /**
     * COP commands
     * @see p. 4.1.3.3 of TC Data Link protocol
     */
    inline constexpr uint8_t UnlockCommandSize = 1; // in octets
    inline constexpr uint8_t UnlockCommandOctet = 0; // all zeroes
    inline constexpr uint8_t SetVrCommandSize = 3; // in octets
    inline constexpr uint8_t SetVrCommandOctet1 = 0x82;
    inline constexpr uint8_t SetVrCommandOctet2 = 0; // Octet 3 carries the new Vr

    /**
     * CLCW fields
     * @see p. 4.2 of TC Data Link Protocol
     */
    inline constexpr uint8_t ControlWordTypeCLCW = 0x00; // the value of 0 ('Type 1 control word type') indicates that clcws are carried as a report word
    inline constexpr uint8_t ClcwVersionNumber = 0x0; // '00' is the only available value currently
    inline constexpr uint8_t CopInEffect = 0x01; // Indicates COP-1 is used

    /**
     * @see p. 5.1.2 from COP-1 CCSDS
     */
    enum class FOPState : uint8_t {
        ACTIVE = 1,
        RETRANSMIT_WITHOUT_WAIT = 2,
        RETRANSMIT_WITH_WAIT = 3,
        INITIALIZING_WITHOUT_BC_FRAME = 4,
        INITIALIZING_WITH_BC_FRAME = 5,
        INITIAL = 6
    };

    /**
     * @see p. 5.1.11 from COP-1 CCSDS
     */
    enum class SuspendVariableState : uint8_t {
        NOT_SUSPENDED = 0,
        SUSPENDED_PREV_STATE_ACTIVE = 1,
        SUSPENDED_PREV_STATE_RETRANSMIT_WITHOUT_WAIT = 2,
        SUSPENDED_PREV_STATE_RETRANSMIT_WITH_WAIT = 3,
        SUSPENDED_PREV_STATE_INITIALIZING_WITHOUT_BC_FRAME = 4
    };

    /**
     * Defines the behavior of FOP-1 when the countdown timer expires and the maximum number of retransmissions has been reached.
     *
     * - ALERT: Triggers an 'Alert' notification, indicating an unrecoverable condition and the termination of the Sequence-Controlled Service guarantee.
     * - SUSPEND: Suspends FOP-1 operation, allowing it to resume later when conditions permit (e.g., upon receiving a CLCW).
     *
     * The SUSPEND option is recommended for:
     * - High-error-rate links: Prevents premature termination during transient issues.
     * - Deep-space links: Accommodates high latency by allowing a 'send and suspend' approach.
     */
    enum class TimeoutType : bool {
        ALERT = 0,
        SUSPEND = 1,
    };

    /**
     * @}
     */

    /** ======================
     *   @name SDLS Constants
     *  ======================
     *  @{
     */

    using Spi = uint16_t; // The security parameter index is a 2 byte id, uniquely identifying a security association

    inline constexpr uint8_t securityParameterIndexLength = 2;
    inline constexpr uint8_t MaxAuthenticationKeyLength = 64;
    inline constexpr uint8_t MaxInitializationVectorLength = 32;
    inline constexpr uint8_t MaxSequenceNumberLength = 8;
    inline constexpr uint8_t MinMACLength = 8;
    inline constexpr uint8_t MaxMACLength = 32;

    enum class AuthenticationAlgorithm : uint8_t {
        NO_AUTHENTICATION,
        HMAC_SHA256_40_BIT
    };

    enum class EncryptionAlgorithm : uint8_t {
        NO_ENCRYPTION
    };

    enum class SecurityAssociationStatus: bool {
        PAUSED,
        RUNNING
    };

    /**
     * @}
     */

    /** ========================
     *   @name Logger constants
     *  ========================
     *  @{
     */
    inline constexpr uint8_t logVerbose = 0;

    /**
     * @}
     */

    /** ========================
     *   @name Other
     *  ========================
     *  @{
     */
    inline constexpr uint32_t MutexDelayMs = 200;
    inline constexpr uint8_t MaxNumberOfMasterChannelsUnderVirtualChannel = 2;

    /**
     * Utility struct for re-constructing segmented packets
     */
    struct SegmentedPacketConstructorTc {
        SegmentedPacketConstructorTc(uint16_t maxExpectedPacketSize) : previousFrameSeqFlag(SequenceFlag::NO_SEGMENTATION),
        segmentedPacketRejectionMode(false), maxExpectedPacketSize(maxExpectedPacketSize) {}

        /**
         * @brief Allocate space for the queue that will hold the reconstructed packet. Do
         * not use the struct if this function is not called first
         */
        void initializeQueue(etl::span<uint8_t> segmentedPacketBuff) {
           segmentedPacket = Queue(segmentedPacketBuff);
        }

        bool addNextSegementedPacketPiece(etl::span<uint8_t> packetPiece) {
            if (segmentedPacket.currentSize() > maxExpectedPacketSize ||
                segmentedPacket.currentSize() + packetPiece.size() > maxExpectedPacketSize) {
                return false;
            }

            for (uint8_t octet : packetPiece) {
                segmentedPacket.push(octet);
            }
            return true;
        }

        bool extractPacket(uint8_t* destBuffer) {
            if (segmentedPacket.currentSize() == 0) {
                return false;
            }

            for (uint16_t i = 0; i < segmentedPacket.currentSize(); i++) {
                destBuffer[i] = segmentedPacket.getFront();
                segmentedPacket.pop();
            }
            return true;
        }

        /**
         * @brief Used to reset buffer in case of fault
         */
        void resetPacket() {
            segmentedPacket.reset();
        }

        SequenceFlag previousFrameSeqFlag;

        /**
         * @brief Is set to true if a wrong sequence flag is encountered mid-packet construction, or
         *        packet exceeded max expected length. Is set back to false again, when a SEGMENTATION_START or
         *        NO_SEGMENTATION flag has arrived
         */
        bool segmentedPacketRejectionMode;

    private:
        Queue<uint8_t> segmentedPacket;
        const uint16_t maxExpectedPacketSize;
    };
    /**
     * @}
     */
} // namespace Defs
