#pragma once

#include <cstdint>
#include "map"
#include <string>
#include "Alert.hpp"
#include "TransferFrameTC.hpp"
std::ostream& operator<<(std::ostream& out, const TxRx value);
std::ostream& operator<<(std::ostream& out, const NotificationType value);
std::ostream& operator<<(std::ostream& out, const ServiceChannelNotification value);
std::ostream& operator<<(std::ostream& out, const COPDirectiveResponse value);
std::ostream& operator<<(std::ostream& out, const FOPNotification value);
