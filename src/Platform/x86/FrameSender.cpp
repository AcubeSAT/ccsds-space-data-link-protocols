#include "FrameSender.hpp"

FrameSender::FrameSender() {
    socket = ::socket(AF_INET, SOCK_STREAM, 0);
    destination.sin_family = AF_INET;
    destination.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &destination.sin_addr);

    //TCP Socket Binding
    for(;;) {
        if (bind(socket, (sockaddr*)&destination, sizeof(destination)) < 0) {
            printf("\nTCP socket binding failed\n");
            sleep(1);
        }
        else {
            printf("\nBinding with 10014 finished successfully\n");
            break;
            //LOG_DEBUG <<"Binding with 10014 finished successfully";
        }
    }

    //TCP Socket Listening
    if(listen(socket,1) <0){
        printf("\nTCP socket listening failed\n");
    }
    else{
        printf("\nListening\n");
        //LOG_DEBUG <<"Listening";
    }
    //TCP Socket Accept
    clientSocket = accept(socket,(sockaddr*)&destination, (socklen_t*)&destination);
    if (clientSocket<0){
        printf("\nTCP socket acceptance failed\n");
    }
    else{
        printf("\nAccepted\n");
        sleep(2);
        //sleep(15);
        //LOG_DEBUG <<"Accepted";
    }

}

FrameSender::~FrameSender() {

}

void FrameSender::sendFrameToYamcs(uint8_t* frame, uint16_t frameLength) {
    String<TmTransferFrameSize> createdFrame(frame, frameLength);
    auto bytesSent = ::send(clientSocket, createdFrame.c_str(), createdFrame.length(), MSG_NOSIGNAL);
    LOG_DEBUG << bytesSent << " bytes sent";
}
