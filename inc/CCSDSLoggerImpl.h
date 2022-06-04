#ifndef CCSDS_TM_PACKETS_CCSDSLOGGERIMPL_H
#define CCSDS_TM_PACKETS_CCSDSLOGGERIMPL_H
#include <iostream>
#include <iomanip>
#include <logOperators.h>
#include "Logger.hpp"

/**
 * Allows to log additional data of interest
 */
template <typename T, class myNotif>
void ccsdsLog(TxRx txRx, NotificationType notificationType, myNotif Notif, T message){
    std::ostringstream ss;
    ss << txRx << ":" << notificationType << ":" << Notif << ":" << message;
    LOG_NOTICE << txRx << ":" << notificationType << ":" << Notif << ":" << message;
}
template <class myNotif>
void ccsdsLog(TxRx txRx, NotificationType notificationType, myNotif Notif) {
	std::ostringstream ss;
	switch (LOG_VERBOSE) {
		case 0:
			ss << txRx << ":" << notificationType << ":" << Notif;
			LOG_NOTICE << ss.str();
			break;
		case 1:
			uint16_t a;
			a = (static_cast<uint16_t>(txRx) << 8U) | (static_cast<uint16_t>(notificationType) << 5U) |
			    (static_cast<uint16_t>(Notif));
			ss << std::hex << a << std::endl;
			LOG_NOTICE << ss.str();
			break;
	};
}

#endif // CCSDS_TM_PACKETS_CCSDSLOGGERIMPL_H
