#pragma once

#include "CCSDSChannel.hpp"
#include "TransferFrameTM.hpp"
#include <arpa/inet.h>
#include "iomanip"
#include "iostream"
#include "netinet/in.h"
#include "sys/socket.h"
#include "unistd.h"
#include "Logger.hpp"

class FrameSender{
private:
    const uint16_t port = 10014;
    struct sockaddr_in destination;
    int socket;
    int clientSocket;

public:
    FrameSender();
    ~FrameSender();

    void sendFrameToYamcs(uint8_t* frame, uint16_t frameLength);



};
