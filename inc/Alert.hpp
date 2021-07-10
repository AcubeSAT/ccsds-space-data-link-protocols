#ifndef CCSDS_ALERT_HPP
#define CCSDS_ALERT_HPP

enum ServiceChannelNotif {
	NO_SERVICE_EVENT,
	MAP_CHANNEL_FRAME_BUFFER_FULL,
	VC_MC_FRAME_BUFFER_FULL,
	TX_MC_FRAME_BUFFER_FULL,
	NO_TX_PACKETS_TO_PROCESS,
	NO_RX_PACKETS_TO_PROCESS,
	PACKET_EXCEEDS_MAX_SIZE,
	FOP_SENT_QUEUE_FULL,
	TX_TO_BE_TRANSMITTED_FRAMES_LIST_EMPTY,
	FOP_REQUEST_REJECTED,
	RX_IN_MC_FULL,
	RX_IN_BUFFER_FULL,
	RX_OUT_BUFFER_FULL,
	RX_INVALID_TFVN,
	RX_INVALID_SCID,
	RX_INVALID_LENGTH,
#if tc_error_control_field_exists
	RX_INVALID_CRC,
#endif
};

enum FOPNotif {
	NO_FOP_EVENT,
	SENT_QUEUE_FULL,
};

enum FOPDirectiveResponse {
	ACCEPT,
	REJECT,
};

enum FOPConfirmResponse {
	POSITIVE,
	NEGATIVE,
};

enum VirtualChannelAlert {
	NO_VC_ALERT,
	UNPROCESSED_PACKET_LIST_FULL,
	WAIT_QUEUE_FULL,
};

enum MasterChannelAlert {
	NO_MC_ALERT,
	OUT_FRAMES_LIST_FULL,
	TO_BE_TRANSMITTED_FRAMES_LIST_FULL,
	MAX_AMOUNT_OF_VIRT_CHANNELS,
};

#endif // CCSDS_ALERT_HPP
