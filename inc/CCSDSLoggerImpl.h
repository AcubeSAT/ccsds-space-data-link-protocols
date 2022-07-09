#ifndef CCSDS_TM_PACKETS_CCSDSLOGGERIMPL_H
#define CCSDS_TM_PACKETS_CCSDSLOGGERIMPL_H
#include <iostream>
#include <iomanip>
#include <logOperators.h>

#include "etl/basic_string.h"
#include "Logger.hpp"


/**
 * Allows to log additional data of interest
 */
template <typename T, class CCSDSNotification>
void ccsdsLogNotice(TxRx txRx, NotificationType notificationType, CCSDSNotification Notif, T message){
	LOG_NOTICE << txRx << ":" << notificationType << ":" << Notif << ":" << message;
}

template <class CCSDSNotification>
void ccsdsLogNotice(TxRx txRx, NotificationType notificationType, CCSDSNotification Notif) {
	switch (LOG_VERBOSE) {
		case 0:
            LOG_NOTICE << txRx << ":" << notificationType << ":" << Notif;
            break;
		case 1:
			uint16_t a;
			a = (static_cast<uint16_t>(txRx) << 8U) | (static_cast<uint16_t>(notificationType) << 5U) |
			    (static_cast<uint16_t>(Notif));
			LOG_NOTICE << std::hex << a;
			break;
	};
}

#endif // CCSDS_TM_PACKETS_CCSDSLOGGERIMPL_H
