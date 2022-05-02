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
		INSERT_ELEMENT(TypeCOPDirectiveResponse);
		INSERT_ELEMENT(TypeFOPNotif);
		INSERT_ELEMENT(TypeFDURequestType);
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
		INSERT_ELEMENT(FOP_SENT_QUEUE_FULL);
		INSERT_ELEMENT(TX_TO_BE_TRANSMITTED_FRAMES_LIST_EMPTY);
		INSERT_ELEMENT(TX_TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
		INSERT_ELEMENT(FOP_REQUEST_REJECTED);
		INSERT_ELEMENT(RX_IN_MC_FULL);
		INSERT_ELEMENT(RX_IN_BUFFER_FULL);
		INSERT_ELEMENT(RX_OUT_BUFFER_FULL);
		INSERT_ELEMENT(RX_INVALID_TFVN);
		INSERT_ELEMENT(RX_INVALID_SCID);
		INSERT_ELEMENT(RX_INVALID_LENGTH);
		INSERT_ELEMENT(VC_RX_WAIT_QUEUE_FULL);
		INSERT_ELEMENT(TX_FOP_REJECTED);
#undef INSERT_ELEMENT
	}
	return out << strings[value];
}

std::ostream& operator<<(std::ostream& out, const COPDirectiveResponse value) {
	static std::map<COPDirectiveResponse, std::string> strings;
	if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
		INSERT_ELEMENT(ACCEPT);
		INSERT_ELEMENT(REJECT);
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
		INSERT_ELEMENT(WAIT_QUEUE_EMPTY);
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

std::ostream& operator<<(std::ostream& out, const FDURequestType value) {
	static std::map<FDURequestType, std::string> strings;
	if (strings.empty()) {
#define INSERT_ELEMENT(p) strings[p] = #p
		INSERT_ELEMENT(REQUEST_PENDING);
		INSERT_ELEMENT(REQUEST_POSITIVE_CONFIRM);
		INSERT_ELEMENT(REQUEST_NEGATIVE_CONFIRM);
#undef INSERT_ELEMENT
	}
	return out << strings[value];
}
