#pragma once

#include <iostream>
#include <iomanip>
#include "logOperators.h"
#include "Logger.hpp"

namespace CCSDSDataLinkLayer {
/**
 * Allows to log additional data of interest
 *
 * @note The CCSDS standards define the error handling procedures of the CCSDS Services. Currently,
 * the logger is simply used for extra observability and that is why the log level is restricted to
 * that of a notice
 */
    template<typename T, class CCSDSNotification>
    void ccsdsLogNotice(TxRx txRx, NotificationType notificationType, CCSDSNotification Notif, T message) {
        LOG_NOTICE << static_cast<uint16_t>(txRx) << ":" << static_cast<uint16_t>(notificationType) << ":" <<
        static_cast<uint16_t>(Notif) << ":" << message;
    }

    template<class CCSDSNotification>
    void ccsdsLogNotice(TxRx txRx, NotificationType notificationType, CCSDSNotification Notif) {
        switch (Defs::logVerbose) {
            case 0:
                LOG_NOTICE << static_cast<uint16_t>(txRx) << ":" << static_cast<uint16_t>(notificationType) << ":" <<
                static_cast<uint16_t>(Notif);
                break;
            case 1:
                uint16_t a;
                a = (static_cast<uint16_t>(txRx) << 8U) | (static_cast<uint16_t>(notificationType) << 5U) |
                    (static_cast<uint16_t>(Notif));
                LOG_NOTICE << std::hex << a;
                break;
        };
    }
} // namespace CCSDSDataLinkLayer