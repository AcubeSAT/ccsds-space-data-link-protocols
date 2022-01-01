#ifndef CCSDS_TM_PACKETS_CCSDS_LOG_H
#define CCSDS_TM_PACKETS_CCSDS_LOG_H
#include <cstdint>
#include "map"
#include <string>
#include "Logger.hpp"
#include "Alert.hpp"
#include "PacketTC.hpp"

enum TxRx : uint8_t  {
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


void ccsdsLog(TxRx txRx, NotificationType notificationType, ServiceChannelNotification serviceChannelNotif);
void ccsdsLog(TxRx txRx, NotificationType notificationType, MasterChannelAlert serviceChannelNotif);
void ccsdsLog(TxRx txRx, NotificationType notificationType, VirtualChannelAlert serviceChannelNotif);
void ccsdsLog(TxRx txRx, NotificationType notificationType, FOPNotification serviceChannelNotif);
void ccsdsLog(TxRx txRx, NotificationType notificationType, FDURequestType serviceChannelNotif);
void ccsdsLog(TxRx txRx, NotificationType notificationType, COPDirectiveResponse serviceChannelNotif);
std::ostream& operator<<(std::ostream& out, const TxRx value);
std::ostream& operator<<(std::ostream& out, const NotificationType value);
std::ostream& operator<<(std::ostream& out, const ServiceChannelNotification value);
std::ostream& operator<<(std::ostream& out, const COPDirectiveResponse value);
std::ostream& operator<<(std::ostream& out, const FOPNotification value);

#endif // CCSDS_TM_PACKETS_CCSDS_LOG_H
