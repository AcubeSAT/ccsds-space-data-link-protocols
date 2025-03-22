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
        NO_SERVICE_EVENT = 0x01,
        MAP_CHANNEL_FRAME_BUFFER_FULL = 0x02,
        MASTER_CHANNEL_FRAME_BUFFER_FULL = 0x03,
        VC_MC_FRAME_BUFFER_FULL = 0x04,
        TX_MC_FRAME_BUFFER_FULL = 0x05,
        NO_TX_PACKETS_TO_PROCESS = 0x06,
        NO_RX_PACKETS_TO_PROCESS = 0x07,
        PACKET_EXCEEDS_MAX_SIZE = 0x08,
        TX_TO_BE_TRANSMITTED_FRAMES_LIST_EMPTY = 0x0A,
        TX_TO_BE_TRANSMITTED_FRAMES_LIST_FULL = 0x0B,
        RX_IN_MC_FULL = 0x0D,
        RX_IN_BUFFER_FULL = 0x0E,
        RX_OUT_BUFFER_FULL = 0x0F,
        RX_INVALID_TFVN = 0x10,
        RX_INVALID_SCID = 0x11,
        RX_INVALID_LENGTH = 0x12,
        VC_RX_WAIT_QUEUE_FULL = 0x13,
        VC_MC_FRAME_BUFFER_EMPTY = 0x15,
        INVALID_VC_ID = 0x16,
        INVALID_MAP_ID = 0x17,
        RX_INVALID_CRC = 0x1A,
        INVALID_SERVICE_CALL = 0x1B,
        PACKET_BUFFER_EMPTY = 0x1C,
        NO_TX_PACKETS_TO_TRANSFER_FRAME = 0x1D,
        MC_RX_INVALID_COUNT = 0x1E,
        MEMORY_POOL_FULL = 0x1F,
        INVALID_INPUT = 0x20,
        SDLS_ERROR = 0x21,
        FOP_BUFFER_FULL = 0x22,
        FOP_BUFFER_EMPTY = 0x23,
        INVALID_SERVICE_TYPE = 0x24,
        FOP_ERROR = 0x25,
        UNEXPECTED_FOP_RETURN_SIGNAL = 0x26,
        CLCW_BUFFER_EMPTY = 0x27,
        PROCESSING_SEGMENTED_PACKET = 0x28,
        INVALID_SEQUENCE_FLAG = 0x29,
        UNKNOWN_ERROR = 0x2A,
        FARM_ERROR = 0x2B,
        GOT_INVALID_MAC_OR_ANTIREPLAY_SEQ_NUMBER = 0x2C
    };

    enum class MapChannelAlert : uint8_t {
	    PROCESSING_LIST_FULL = 0x01,
        PROCESSING_LIST_EMPTY = 0x02,
	    INVALID_SERVICE_TYPE = 0x03,
	    REQUSTED_SERVICE_TYPE_FRAME_NOT_FOUND = 0x04,
        PACKET_QUEUE_FULL = 0x05,
        PACKET_QUEUE_EMPTY = 0x06,
    };

    enum class VirtualChannelAlert : uint8_t {
        NO_VC_ALERT = 0x01,
        UNPROCESSED_PACKET_LIST_FULL = 0x02,
        TX_WAIT_QUEUE_FULL = 0x03,
        RX_WAIT_QUEUE_FULL = 0x04,
	    MAX_AMOUNT_OF_MAP_CHANNELS = 0x05,
    };

    enum class MasterChannelAlert : uint8_t {
        NO_MC_ALERT = 0x01,
        OUT_FRAMES_LIST_FULL = 0x02,
        TO_BE_TRANSMITTED_FRAMES_LIST_FULL = 0x03,
        MAX_AMOUNT_OF_VIRT_CHANNELS = 0x04,
        NO_SPACE = 0x05
    };

    enum class SDLSVerificationStatusCode : uint8_t {
        NO_FAILURE = 0x01,
        INVALID_SPI = 0x02,
        INVALID_FRAME_TYPE = 0x03,
        UNASSOCIATED_CHANNEL = 0x04,
        INVALID_USER = 0x05,
        MAC_CALCULATION_ERROR = 0x06,
        MAC_VERIFICATION_FAILURE = 0x07,
        ANTI_REPLAY_SEQUENCE_NUMBER_FAILURE = 0x08,
        PADDING_ERROR = 0x09
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