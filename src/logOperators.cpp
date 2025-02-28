#include <logOperators.h>
#include "Alert.hpp"

namespace CCSDSDataLinkLayer {
    std::ostream &operator<<(std::ostream &out, const TxRx value) {
        static std::map<TxRx, std::string> strings;
        if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
            INSERT_ELEMENT(TxRx::Tx);
            INSERT_ELEMENT(TxRx::Rx);
#undef INSERT_ELEMENT
        }
        return out << strings[value];
    }

    std::ostream &operator<<(std::ostream &out, const NotificationType value) {
        static std::map<NotificationType, std::string> strings;
        if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
            INSERT_ELEMENT(NotificationType::TypeVirtualChannelAlert);
            INSERT_ELEMENT(NotificationType::TypeMasterChannelAlert);
            INSERT_ELEMENT(NotificationType::TypeServiceChannelNotif);
            INSERT_ELEMENT(NotificationType::TypeFDURequestType);
            INSERT_ELEMENT(NotificationType::TypeSDLSVerificationStatusCode);
            INSERT_ELEMENT(NotificationType::TypeFOPNotif);
            INSERT_ELEMENT(NotificationType::TypeFARMNotif);
#undef INSERT_ELEMENT
        }
        return out << strings[value];
    }

    std::ostream &operator<<(std::ostream &out, const ServiceChannelNotification value) {
        static std::map<ServiceChannelNotification, std::string> strings;
        if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
            INSERT_ELEMENT(ServiceChannelNotification::NO_SERVICE_EVENT);
            INSERT_ELEMENT(ServiceChannelNotification::MAP_CHANNEL_FRAME_BUFFER_FULL);
            INSERT_ELEMENT(ServiceChannelNotification::MASTER_CHANNEL_FRAME_BUFFER_FULL);
            INSERT_ELEMENT(ServiceChannelNotification::VC_MC_FRAME_BUFFER_FULL);
            INSERT_ELEMENT(ServiceChannelNotification::TX_MC_FRAME_BUFFER_FULL);
            INSERT_ELEMENT(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS);
            INSERT_ELEMENT(ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS);
            INSERT_ELEMENT(ServiceChannelNotification::PACKET_EXCEEDS_MAX_SIZE);
            INSERT_ELEMENT(ServiceChannelNotification::TX_TO_BE_TRANSMITTED_FRAMES_LIST_EMPTY);
            INSERT_ELEMENT(ServiceChannelNotification::TX_TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
            INSERT_ELEMENT(ServiceChannelNotification::RX_IN_MC_FULL);
            INSERT_ELEMENT(ServiceChannelNotification::RX_IN_BUFFER_FULL);
            INSERT_ELEMENT(ServiceChannelNotification::RX_OUT_BUFFER_FULL);
            INSERT_ELEMENT(ServiceChannelNotification::RX_INVALID_TFVN);
            INSERT_ELEMENT(ServiceChannelNotification::RX_INVALID_SCID);
            INSERT_ELEMENT(ServiceChannelNotification::RX_INVALID_LENGTH);
            INSERT_ELEMENT(ServiceChannelNotification::VC_RX_WAIT_QUEUE_FULL);
            INSERT_ELEMENT(ServiceChannelNotification::VC_MC_FRAME_BUFFER_EMPTY);
            INSERT_ELEMENT(ServiceChannelNotification::INVALID_VC_ID);
            INSERT_ELEMENT(ServiceChannelNotification::INVALID_MAP_ID);
            INSERT_ELEMENT(ServiceChannelNotification::RX_INVALID_CRC);
            INSERT_ELEMENT(ServiceChannelNotification::INVALID_SERVICE_CALL);
            INSERT_ELEMENT(ServiceChannelNotification::PACKET_BUFFER_EMPTY);
            INSERT_ELEMENT(ServiceChannelNotification::NO_TX_PACKETS_TO_TRANSFER_FRAME);
            INSERT_ELEMENT(ServiceChannelNotification::MC_RX_INVALID_COUNT);
            INSERT_ELEMENT(ServiceChannelNotification::MEMORY_POOL_FULL);
            INSERT_ELEMENT(ServiceChannelNotification::INVALID_INPUT);
            INSERT_ELEMENT(ServiceChannelNotification::SDLS_ERROR);
            INSERT_ELEMENT(ServiceChannelNotification::FOP_BUFFER_FULL);
            INSERT_ELEMENT(ServiceChannelNotification::FOP_BUFFER_EMPTY);
            INSERT_ELEMENT(ServiceChannelNotification::INVALID_SERVICE_TYPE);
            INSERT_ELEMENT(ServiceChannelNotification::FOP_ERROR);
            INSERT_ELEMENT(ServiceChannelNotification::UNEXPECTED_FOP_RETURN_SIGNAL);
            INSERT_ELEMENT(ServiceChannelNotification::CLCW_BUFFER_EMPTY);
            INSERT_ELEMENT(ServiceChannelNotification::PROCESSING_SEGMENTED_PACKET);
            INSERT_ELEMENT(ServiceChannelNotification::INVALID_SEQUENCE_FLAG);
            INSERT_ELEMENT(ServiceChannelNotification::UNKNOWN_ERROR);
            INSERT_ELEMENT(ServiceChannelNotification::FARM_ERROR);
            INSERT_ELEMENT(ServiceChannelNotification::GOT_INVALID_MAC_OR_ANTIREPLAY_SEQ_NUMBER);
#undef INSERT_ELEMENT
        }
        return out << strings[value];
    }

    std::ostream &operator<<(std::ostream &out, const FOPNotification value) {
        static std::map<FOPNotification, std::string> strings;
        if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
            INSERT_ELEMENT(FOPNotification::NO_FOP_EVENT);
            INSERT_ELEMENT(FOPNotification::SENT_QUEUE_FULL);
            INSERT_ELEMENT(FOPNotification::SENT_QUEUE_EMPTY);
            INSERT_ELEMENT(FOPNotification::WAIT_QUEUE_FULL);
            INSERT_ELEMENT(FOPNotification::WAIT_QUEUE_EMPTY);
            INSERT_ELEMENT(FOPNotification::SIGNAL_QUEUE_FULL);
            INSERT_ELEMENT(FOPNotification::SIGNAL_QUEUE_EMPTY);
            INSERT_ELEMENT(FOPNotification::FOP_MEMORY_POOL_FULL);
            INSERT_ELEMENT(FOPNotification::FOP_MASTER_COPY_BUFFER_FULL);
            INSERT_ELEMENT(FOPNotification::FOP_NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT);
            INSERT_ELEMENT(FOPNotification::FOP_UNEXPECTED_VALUE);
#undef INSERT_ELEMENT
        }
        return out << strings[value];
    }

    std::ostream &operator<<(std::ostream &out, const FARMNotification value) {
        static std::map<FARMNotification, std::string> strings;
        if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
            INSERT_ELEMENT(FARMNotification::NO_FARM_EVENT);
            INSERT_ELEMENT(FARMNotification::FARM_NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT);
            INSERT_ELEMENT(FARMNotification::FARM_UNEXPECTED_VALUE);
            INSERT_ELEMENT(FARMNotification::FARM_HIGH_LAYER_AD_BUFFER_FULL);
#undef INSERT_ELEMENT
        }
        return out << strings[value];
    }

    std::ostream &operator<<(std::ostream &out, const MasterChannelAlert value) {
        static std::map<MasterChannelAlert, std::string> strings;
        if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
            INSERT_ELEMENT(MasterChannelAlert::NO_MC_ALERT);
            INSERT_ELEMENT(MasterChannelAlert::OUT_FRAMES_LIST_FULL);
            INSERT_ELEMENT(MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
            INSERT_ELEMENT(MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS);
            INSERT_ELEMENT(MasterChannelAlert::NO_SPACE);
#undef INSERT_ELEMENT
        }
        return out << strings[value];
    }

    std::ostream &operator<<(std::ostream &out, const VirtualChannelAlert value) {
        static std::map<VirtualChannelAlert, std::string> strings;
        if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
            INSERT_ELEMENT(VirtualChannelAlert::NO_VC_ALERT);
            INSERT_ELEMENT(VirtualChannelAlert::UNPROCESSED_PACKET_LIST_FULL);
            INSERT_ELEMENT(VirtualChannelAlert::TX_WAIT_QUEUE_FULL);
            INSERT_ELEMENT(VirtualChannelAlert::RX_WAIT_QUEUE_FULL);
#undef INSERT_ELEMENT
        }
        return out << strings[value];
    }

    std::ostream &operator<<(std::ostream &out, const SDLSVerificationStatusCode value) {
        static std::map<SDLSVerificationStatusCode, std::string> strings;
        if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
            INSERT_ELEMENT(SDLSVerificationStatusCode::NO_FAILURE);
            INSERT_ELEMENT(SDLSVerificationStatusCode::INVALID_SPI);
            INSERT_ELEMENT(SDLSVerificationStatusCode::INVALID_FRAME_TYPE);
            INSERT_ELEMENT(SDLSVerificationStatusCode::UNASSOCIATED_CHANNEL);
            INSERT_ELEMENT(SDLSVerificationStatusCode::INVALID_USER);
            INSERT_ELEMENT(SDLSVerificationStatusCode::MAC_CALCULATION_ERROR);
            INSERT_ELEMENT(SDLSVerificationStatusCode::MAC_VERIFICATION_FAILURE);
            INSERT_ELEMENT(SDLSVerificationStatusCode::ANTI_REPLAY_SEQUENCE_NUMBER_FAILURE);
            INSERT_ELEMENT(SDLSVerificationStatusCode::PADDING_ERROR);
#undef INSERT_ELEMENT
        }
        return out << strings[value];
    }
} // namespace CCSDSDataLinkLayer