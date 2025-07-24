#pragma once
#include "Alert.hpp"

namespace CCSDSDataLinkLayer {
    template<class MyNotif>
    void ccsdsLogNotice(TxRx txRx, NotificationType notificationType, MyNotif Notif);
} // namespace CCSDSDataLinkLayer
