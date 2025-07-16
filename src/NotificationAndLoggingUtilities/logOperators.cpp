#include "logOperators.h"
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
		    INSERT_ELEMENT(NotificationType::TypeMAPChannelAlert);
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
        INSERT_ELEMENT(ServiceChannelNotification::NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL);
        INSERT_ELEMENT(ServiceChannelNotification::INVALID_TFVN);
        INSERT_ELEMENT(ServiceChannelNotification::INVALID_SCID);
        INSERT_ELEMENT(ServiceChannelNotification::INVALID_LENGTH);
        INSERT_ELEMENT(ServiceChannelNotification::INVALID_CRC);
        INSERT_ELEMENT(ServiceChannelNotification::INVALID_VCID);
        INSERT_ELEMENT(ServiceChannelNotification::INVALID_MAPID);
        INSERT_ELEMENT(ServiceChannelNotification::INVALID_CHANNELS_COMBINATION);
        INSERT_ELEMENT(ServiceChannelNotification::FARM_ERROR);
        INSERT_ELEMENT(ServiceChannelNotification::FRAME_QUEUE_FULL);
        INSERT_ELEMENT(ServiceChannelNotification::FRAME_QUEUE_EMPTY);
        INSERT_ELEMENT(ServiceChannelNotification::REQUESTED_FRAME_TYPE_NOT_FOUND);
        INSERT_ELEMENT(ServiceChannelNotification::PACKET_QUEUE_FULL);
        INSERT_ELEMENT(ServiceChannelNotification::PACKET_QUEUE_EMPTY);
        INSERT_ELEMENT(ServiceChannelNotification::UNASSOCIATED_CHANNEL);
        INSERT_ELEMENT(ServiceChannelNotification::INVALID_SECURITY_PARAMETER_INDEX);
        INSERT_ELEMENT(ServiceChannelNotification::INVALID_MAC);
        INSERT_ELEMENT(ServiceChannelNotification::FRAME_REPLAY_ATTEMPT);
        INSERT_ELEMENT(ServiceChannelNotification::SLDS_ERROR);
        INSERT_ELEMENT(ServiceChannelNotification::OCF_SDU_QUEUE_EMPTY);
        INSERT_ELEMENT(ServiceChannelNotification::INVALID_PACKET_PVN);
        INSERT_ELEMENT(ServiceChannelNotification::NO_SERVICE_EVENT);
        INSERT_ELEMENT(ServiceChannelNotification::DISCARDED_FRAME);
        INSERT_ELEMENT(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
        INSERT_ELEMENT(ServiceChannelNotification::INVALID_CHANNEL_NAME);
        INSERT_ELEMENT(ServiceChannelNotification::UNSUPPORTED_SERVICE_TYPE);
        INSERT_ELEMENT(ServiceChannelNotification::OCF_SDU_QUEUE_FULL);
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
            INSERT_ELEMENT(FOPNotification::FOP_MEMORY_POOL_OR_MASTER_COPY_BUFFER_FULL);
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
            INSERT_ELEMENT(MasterChannelAlert::FRAME_LIST_FULL);
            INSERT_ELEMENT(MasterChannelAlert::FRAME_LIST_EMPTY);
            INSERT_ELEMENT(MasterChannelAlert::REQUESTED_FRAME_NOT_FOUND);
            INSERT_ELEMENT(MasterChannelAlert::MASTER_COPY_BUFFER_FULL);
            INSERT_ELEMENT(MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL);
#undef INSERT_ELEMENT
        }
        return out << strings[value];
    }

    std::ostream &operator<<(std::ostream &out, const VirtualChannelAlert value) {
        static std::map<VirtualChannelAlert, std::string> strings;
        if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
            INSERT_ELEMENT(VirtualChannelAlert::FRAME_LIST_FULL);
            INSERT_ELEMENT(VirtualChannelAlert::FRAME_LIST_EMPTY);
            INSERT_ELEMENT(VirtualChannelAlert::INVALID_SERVICE_TYPE);
            INSERT_ELEMENT(VirtualChannelAlert::INVALID_INPUT);
            INSERT_ELEMENT(VirtualChannelAlert::REQUSTED_FRAME_NOT_FOUND);
            INSERT_ELEMENT(VirtualChannelAlert::PACKET_QUEUE_FULL);
            INSERT_ELEMENT(VirtualChannelAlert::PACKET_QUEUE_EMPTY);
            INSERT_ELEMENT(VirtualChannelAlert::INVALID_REQUESTED_PACKET_SEGMENT);
            INSERT_ELEMENT(VirtualChannelAlert::FOP_SIGNAL_QUEUE_FULL);
            INSERT_ELEMENT(VirtualChannelAlert::FOP_SIGNAL_QUEUE_EMPTY);
#undef INSERT_ELEMENT
        }
        return out << strings[value];
    }

	std::ostream &operator<<(std::ostream &out, const MapChannelAlert value) {
	    static std::map<MapChannelAlert, std::string> strings;
	    if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
		    INSERT_ELEMENT(MapChannelAlert::FRAME_LIST_FULL);
	        INSERT_ELEMENT(MapChannelAlert::FRAME_LIST_EMPTY);
		    INSERT_ELEMENT(MapChannelAlert::INVALID_SERVICE_TYPE);
	        INSERT_ELEMENT(MapChannelAlert::REQUSTED_FRAME_NOT_FOUND);
	        INSERT_ELEMENT(MapChannelAlert::PACKET_QUEUE_FULL);
	        INSERT_ELEMENT(MapChannelAlert::PACKET_QUEUE_EMPTY);
	        INSERT_ELEMENT(MapChannelAlert::INVALID_REQUESTED_PACKET_SEGMENT);
#undef INSERT_ELEMENT
	    }
	    return out << strings[value];
    }

    std::ostream &operator<<(std::ostream &out, const SDLSVerificationError value) {
        static std::map<SDLSVerificationError, std::string> strings;
        if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
            INSERT_ELEMENT(SDLSVerificationError::NO_SECURITY_CONFIGURED);
            INSERT_ELEMENT(SDLSVerificationError::INVALID_SPI);
            INSERT_ELEMENT(SDLSVerificationError::INVALID_FRAME_TYPE);
            INSERT_ELEMENT(SDLSVerificationError::MAC_CALCULATION_ERROR);
            INSERT_ELEMENT(SDLSVerificationError::MAC_VERIFICATION_FAILURE);
            INSERT_ELEMENT(SDLSVerificationError::ANTI_REPLAY_SEQUENCE_NUMBER_FAILURE);
            INSERT_ELEMENT(SDLSVerificationError::PADDING_ERROR);
#undef INSERT_ELEMENT
        }
        return out << strings[value];
    }
} // namespace CCSDSDataLinkLayer