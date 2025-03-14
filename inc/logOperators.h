#pragma once

#include <cstdint>
#include <map>
#include <string>
#include "Alert.hpp"
#include "TransferFrameTC.hpp"

namespace CCSDSDataLinkLayer {
    std::ostream &operator<<(std::ostream &out, const TxRx value);

    std::ostream &operator<<(std::ostream &out, const NotificationType value);

    std::ostream &operator<<(std::ostream &out, const ServiceChannelNotification value);

    std::ostream &operator<<(std::ostream &out, const VirtualChannelAlert value);

    std::ostream &operator<<(std::ostream &out, const MasterChannelAlert value);

    std::ostream &operator<<(std::ostream &out, const SDLSVerificationStatusCode value);

    std::ostream &operator<<(std::ostream &out, const FOPNotification value);

    std::ostream &operator<<(std::ostream &out, const FARMNotification value);
} // namespace CCSDSDataLinkLayer