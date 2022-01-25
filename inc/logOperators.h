#ifndef CCSDS_TM_PACKETS_LOGOPERATORS_H
#define CCSDS_TM_PACKETS_LOGOPERATORS_H
#include <cstdint>
#include "map"
#include <string>
#include "Alert.hpp"
#include "PacketTC.hpp"
std::ostream& operator<<(std::ostream& out, const TxRx value);
std::ostream& operator<<(std::ostream& out, const NotificationType value);
std::ostream& operator<<(std::ostream& out, const ServiceChannelNotification value);
std::ostream& operator<<(std::ostream& out, const COPDirectiveResponse value);
std::ostream& operator<<(std::ostream& out, const FOPNotification value);

#endif // CCSDS_TM_PACKETS_LOGOPERATORS_H
