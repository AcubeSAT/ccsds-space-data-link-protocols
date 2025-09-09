#pragma once
#include "DataLinkNotifications.hpp"

namespace CCSDSDataLinkLayer {
    template<class MyNotif>
    void ccsdsLogNotice(TxRx txRx, NotificationType notificationType, MyNotif Notif);
} // namespace CCSDSDataLinkLayer
