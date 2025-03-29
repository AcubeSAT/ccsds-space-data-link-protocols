#pragma once

#include <cstdint>

namespace CCSDSDataLinkLayer {
    enum class TxRx : bool {
        Rx = false,
        Tx = true
    };

    enum class NotificationType : uint8_t {
	    TypeMAPChannelAlert = 0x00,
        TypeVirtualChannelAlert = 0x01,
        TypeMasterChannelAlert = 0x02,
        TypeServiceChannelNotif = 0x03,
        TypeFDURequestType = 0x04,
        TypeSDLSVerificationStatusCode = 0x05,
        TypeFOPNotif = 0x06,
        TypeFARMNotif = 0x07,
    };

    enum class ServiceChannelNotification : uint8_t {
        NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL = 0x01,
        INVALID_TFVN = 0x03,
        INVALID_SCID = 0x04,
        INVALID_LENGTH = 0x05,
        INVALID_CRC = 0x06,
        INVALID_VCID = 0x07,
        INVALID_MAPID = 0x08,
        INVALID_CHANNELS_COMBINATION = 0x09,
        INVALID_SERVICE_TYPE = 0x0A,
        FARM_ERROR = 0x0B,
        FRAME_LIST_FULL = 0x0C,
        FRAME_LIST_EMPTY = 0x0D,
        PACKET_QUEUE_FULL = 0x0E,
        PACKET_QUEUE_EMPTY = 0x0F,
        UNASSOCIATED_CHANNEL = 0x10,
        INVALID_SECURITY_PARAMETER_INDEX = 0x11,
        INVALID_MAC = 0x12,
        FRAME_REPLAY_ATTEMPT = 0x13,
        SLDS_ERROR = 0x14,
        NO_SERVICE_EVENT = 0x15,
    };

    enum class MapChannelAlert : uint8_t {
	    FRAME_LIST_FULL = 0x01,
        FRAME_LIST_EMPTY = 0x02,
	    INVALID_SERVICE_TYPE = 0x03,
	    REQUSTED_FRAME_NOT_FOUND = 0x04,
        PACKET_QUEUE_FULL = 0x05,
        PACKET_QUEUE_EMPTY = 0x06,
        INVALID_REQUESTED_PACKET_SEGMENT = 0x09,
    };

    enum class VirtualChannelAlert : uint8_t {
        FRAME_LIST_FULL = 0x01,
        FRAME_LIST_EMPTY = 0x02,
        INVALID_SERVICE_TYPE = 0x03,
        INVALID_INPUT = 0x04,
        REQUSTED_FRAME_NOT_FOUND = 0x05,
        PACKET_QUEUE_FULL = 0x06,
        PACKET_QUEUE_EMPTY = 0x07,
        INVALID_REQUESTED_PACKET_SEGMENT = 0x08,
        FOP_SIGNAL_QUEUE_FULL = 0x09,
        FOP_SIGNAL_QUEUE_EMPTY = 0x0A,
    };

    enum class MasterChannelAlert : uint8_t {
        NO_MC_ALERT = 0x01,
        FRAME_LIST_FULL = 0x02,
        FRAME_LIST_EMPTY = 0x03,
        REQUESTED_FRAME_NOT_FOUND = 0x04,
        MASTER_COPY_BUFFER_FULL = 0x05,
        NOT_ENOUGH_SPACE_IN_MEMORY_POOL = 0x06,
    };

    enum class SDLSVerificationError : uint8_t {
        NO_SECURITY_CONFIGURED = 0x01,
        INVALID_SPI = 0x02,
        INVALID_FRAME_TYPE = 0x03,
        INVALID_USER = 0x04,
        MAC_CALCULATION_ERROR = 0x05,
        MAC_VERIFICATION_FAILURE = 0x06,
        ANTI_REPLAY_SEQUENCE_NUMBER_FAILURE = 0x07,
        PADDING_ERROR = 0x08
    };

    enum class FOPNotification : uint8_t {
        NO_FOP_EVENT = 0x01,
        SENT_QUEUE_FULL = 0x02,
        SENT_QUEUE_EMPTY = 0x03,
        WAIT_QUEUE_FULL = 0x04,
        WAIT_QUEUE_EMPTY = 0x05,
        SIGNAL_QUEUE_FULL = 0x06,
        SIGNAL_QUEUE_EMPTY = 0x07,
        FOP_MEMORY_POOL_OR_MASTER_COPY_BUFFER_FULL = 0x08,
        FOP_NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT = 0x0A,
        FOP_UNEXPECTED_VALUE = 0x0B,
    };

    enum class FARMNotification : uint8_t {
        NO_FARM_EVENT = 0x01,
        FARM_NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT = 0x02,
        FARM_UNEXPECTED_VALUE = 0x03,
        FARM_HIGH_LAYER_AD_BUFFER_FULL = 0x04,
    };
} // namespace CCSDSDataLInkLayer