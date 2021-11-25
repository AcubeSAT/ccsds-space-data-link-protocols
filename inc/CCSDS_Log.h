#ifndef CCSDS_TM_PACKETS_CCSDS_LOG_H
#define CCSDS_TM_PACKETS_CCSDS_LOG_H
#include <cstdint>
#include "map"
#include <string>
#include "Logger.hpp"
#include "Alert.hpp"
#include "PacketTC.hpp"

enum Tx_Rx: uint8_t  {

	Rx = 0x00,
	Tx = 0x01

};

enum NotificationType: uint8_t {

	TypeVirtualChannelAlert = 0x00,
	TypeMasterChannelAlert = 0x01,
	TypeServiceChannelNotif = 0x02,
	TypeCOPDirectiveResponse = 0x03,
	TypeFOPNotif = 0x04,
	TypeFDURequestType = 0x05

};


void ccsds_log(Tx_Rx tx_rx, NotificationType notification_type, ServiceChannelNotif service_channel_notif);
void ccsds_log(Tx_Rx tx_rx, NotificationType notification_type, MasterChannelAlert service_channel_notif);
void ccsds_log(Tx_Rx tx_rx, NotificationType notification_type, VirtualChannelAlert service_channel_notif);
void ccsds_log(Tx_Rx tx_rx, NotificationType notification_type, FOPNotif service_channel_notif);
void ccsds_log(Tx_Rx tx_rx, NotificationType notification_type, FDURequestType service_channel_notif);
void ccsds_log(Tx_Rx tx_rx, NotificationType notification_type, COPDirectiveResponse service_channel_notif);
std::ostream& operator<<(std::ostream& out, const Tx_Rx value);
std::ostream& operator<<(std::ostream& out, const NotificationType value);
std::ostream& operator<<(std::ostream& out, const ServiceChannelNotif value);
std::ostream& operator<<(std::ostream& out, const COPDirectiveResponse value);
std::ostream& operator<<(std::ostream& out, const FOPNotif value);

#endif // CCSDS_TM_PACKETS_CCSDS_LOG_H
