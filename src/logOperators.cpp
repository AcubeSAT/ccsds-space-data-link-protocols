#include <logOperators.h>

std::ostream& operator<<(std::ostream& out, const TxRx value) {
	static std::map<TxRx, std::string> strings;
	if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
		INSERT_ELEMENT(Tx);
		INSERT_ELEMENT(Rx);
#undef INSERT_ELEMENT
	}
	return out << strings[value];
}

std::ostream& operator<<(std::ostream& out, const NotificationType value) {
	static std::map<NotificationType, std::string> strings;
	if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
		INSERT_ELEMENT(TypeVirtualChannelAlert);
		INSERT_ELEMENT(TypeMasterChannelAlert);
		INSERT_ELEMENT(TypeServiceChannelNotif);
		INSERT_ELEMENT(TypeFDURequestType);
        INSERT_ELEMENT(TypeSDLSVerificationStatusCode);
        INSERT_ELEMENT(TypeFOPNotif);
        INSERT_ELEMENT(TypeFARMNotif);
#undef INSERT_ELEMENT
	}
	return out << strings[value];
}

std::ostream& operator<<(std::ostream& out, const ServiceChannelNotification value) {
	static std::map<ServiceChannelNotification, std::string> strings;
	if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
		INSERT_ELEMENT(NO_SERVICE_EVENT);
		INSERT_ELEMENT(MAP_CHANNEL_FRAME_BUFFER_FULL);
		INSERT_ELEMENT(MASTER_CHANNEL_FRAME_BUFFER_FULL);
		INSERT_ELEMENT(VC_MC_FRAME_BUFFER_FULL);
		INSERT_ELEMENT(TX_MC_FRAME_BUFFER_FULL);
		INSERT_ELEMENT(NO_TX_PACKETS_TO_PROCESS);
		INSERT_ELEMENT(NO_RX_PACKETS_TO_PROCESS);
		INSERT_ELEMENT(PACKET_EXCEEDS_MAX_SIZE);
		INSERT_ELEMENT(TX_TO_BE_TRANSMITTED_FRAMES_LIST_EMPTY);
		INSERT_ELEMENT(TX_TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
		INSERT_ELEMENT(RX_IN_MC_FULL);
		INSERT_ELEMENT(RX_IN_BUFFER_FULL);
		INSERT_ELEMENT(RX_OUT_BUFFER_FULL);
		INSERT_ELEMENT(RX_INVALID_TFVN);
		INSERT_ELEMENT(RX_INVALID_SCID);
		INSERT_ELEMENT(RX_INVALID_LENGTH);
		INSERT_ELEMENT(VC_RX_WAIT_QUEUE_FULL);
        INSERT_ELEMENT(VC_MC_FRAME_BUFFER_EMPTY);
        INSERT_ELEMENT(INVALID_VC_ID);
        INSERT_ELEMENT(INVALID_MAP_ID);
        INSERT_ELEMENT(RX_INVALID_CRC);
        INSERT_ELEMENT(INVALID_SERVICE_CALL);
        INSERT_ELEMENT(PACKET_BUFFER_EMPTY);
        INSERT_ELEMENT(NO_TX_PACKETS_TO_TRANSFER_FRAME);
        INSERT_ELEMENT(MC_RX_INVALID_COUNT);
        INSERT_ELEMENT(MEMORY_POOL_FULL);
        INSERT_ELEMENT(INVALID_INPUT);
        INSERT_ELEMENT(SDLS_ERROR);
        INSERT_ELEMENT(FOP_BUFFER_FULL);
        INSERT_ELEMENT(FOP_BUFFER_EMPTY);
        INSERT_ELEMENT(INVALID_SERVICE_TYPE);
        INSERT_ELEMENT(FOP_ERROR);
        INSERT_ELEMENT(UNEXPECTED_FOP_RETURN_SIGNAL);
        INSERT_ELEMENT(CLCW_BUFFER_EMPTY);
        INSERT_ELEMENT(PROCESSING_SEGMENTED_PACKET);
        INSERT_ELEMENT(INVALID_SEQUENCE_FLAG);
        INSERT_ELEMENT(UNKNOWN_ERROR);
        INSERT_ELEMENT(FARM_ERROR);
        INSERT_ELEMENT(GOT_INVALID_MAC_OR_ANTIREPLAY_SEQ_NUMBER);
#undef INSERT_ELEMENT
	}
	return out << strings[value];
}

std::ostream& operator<<(std::ostream& out, const FOPNotification value) {
	static std::map<FOPNotification, std::string> strings;
	if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
        INSERT_ELEMENT(NO_FOP_EVENT);
        INSERT_ELEMENT(SENT_QUEUE_FULL);
        INSERT_ELEMENT(SENT_QUEUE_EMPTY);
        INSERT_ELEMENT(WAIT_QUEUE_FULL);
        INSERT_ELEMENT(WAIT_QUEUE_EMPTY);
        INSERT_ELEMENT(SIGNAL_QUEUE_FULL);
        INSERT_ELEMENT(SIGNAL_QUEUE_EMPTY);
        INSERT_ELEMENT(FOP_MEMORY_POOL_FULL);
        INSERT_ELEMENT(FOP_MASTER_COPY_BUFFER_FULL);
        INSERT_ELEMENT(FOP_NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT);
        INSERT_ELEMENT(FOP_UNEXPECTED_VALUE);
#undef INSERT_ELEMENT
	}
	return out << strings[value];
}

std::ostream& operator<<(std::ostream& out, const FARMNotification value) {
    static std::map<FARMNotification, std::string> strings;
    if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
        INSERT_ELEMENT(NO_FARM_EVENT);
        INSERT_ELEMENT(FARM_NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT);
        INSERT_ELEMENT(FARM_UNEXPECTED_VALUE);
        INSERT_ELEMENT(FARM_HIGH_LAYER_AD_BUFFER_FULL);
#undef INSERT_ELEMENT
    }
    return out << strings[value];
}

std::ostream& operator<<(std::ostream& out, const MasterChannelAlert value) {
	static std::map<MasterChannelAlert, std::string> strings;
	if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
		INSERT_ELEMENT(NO_MC_ALERT);
		INSERT_ELEMENT(OUT_FRAMES_LIST_FULL);
		INSERT_ELEMENT(TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
		INSERT_ELEMENT(MAX_AMOUNT_OF_VIRT_CHANNELS);
        INSERT_ELEMENT(NO_SPACE);
#undef INSERT_ELEMENT
	}
	return out << strings[value];
}

std::ostream& operator<<(std::ostream& out, const VirtualChannelAlert value) {
	static std::map<VirtualChannelAlert, std::string> strings;
	if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
		INSERT_ELEMENT(NO_VC_ALERT);
		INSERT_ELEMENT(UNPROCESSED_PACKET_LIST_FULL);
		INSERT_ELEMENT(TX_WAIT_QUEUE_FULL);
		INSERT_ELEMENT(RX_WAIT_QUEUE_FULL);
#undef INSERT_ELEMENT
	}
	return out << strings[value];
}

std::ostream& operator<<(std::ostream& out, const SDLSVerificationStatusCode value) {
    static std::map<SDLSVerificationStatusCode, std::string> strings;
    if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
        INSERT_ELEMENT(NO_FAILURE);
        INSERT_ELEMENT(INVALID_SPI);
        INSERT_ELEMENT(INVALID_FRAME_TYPE);
        INSERT_ELEMENT(UNASSOCIATED_CHANNEL);
        INSERT_ELEMENT(INVALID_USER);
        INSERT_ELEMENT(MAC_CALCULATION_ERROR);
        INSERT_ELEMENT(MAC_VERIFICATION_FAILURE);
        INSERT_ELEMENT(ANTI_REPLAY_SEQUENCE_NUMBER_FAILURE);
        INSERT_ELEMENT(PADDING_ERROR);
#undef INSERT_ELEMENT
    }
    return out << strings[value];
}