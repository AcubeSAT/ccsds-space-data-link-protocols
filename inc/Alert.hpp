#ifndef CCSDS_ALERT_HPP
#define CCSDS_ALERT_HPP

typedef enum ServiceChannelNotif {
    NO_SERVICE_EVENT = 0,
    MAP_CHANNEL_FRAME_BUFFER_FULL = 1,
    VIRTUAL_CHANNEL_FRAME_BUFFER_FULL = 2,
    MASTER_CHANNEL_FRAME_BUFFER_FULL = 3,
    NO_PACKETS_TO_PROCESS = 4,
    PACKET_EXCEEDS_MAX_SIZE = 5,
};

typedef enum FOPNotif {
    NO_FOP_EVENT = 0,
    SENT_QUEUE_FULL = 1,
};

typedef enum FOPDirectiveResponse {
    ACCEPT = 0,
    REJECT = 1,
};

typedef enum FOPConfirmResponse {
    POSITIVE = 0,
    NEGATIVE = 1,
};

#endif //CCSDS_ALERT_HPP
