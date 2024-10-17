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
            sleep(2);
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
        sleep(5);
        //sleep(15);
        //LOG_DEBUG <<"Accepted";
    }

}

FrameSender::~FrameSender() {

}

void FrameSender::sendFrameToYamcs(uint8_t* frame, uint16_t frameLength) {
    String<TmTransferFrameSize> createdFrame(frame, frameLength);
	//uint8_t array[] = {10, 176, 1, 1, 24, 0, 8, 1, 195, 39, 0, 76, 32, 4, 2, 1, 70, 0, 1, 37, 165, 61, 202, 14, 224, 184, 148, 14, 224, 185, 92, 0, 2, 19, 152, 0, 3, 64, 160, 0, 0, 14, 224, 185, 92, 63, 128, 0, 0, 14, 224, 185, 92, 64, 64, 0, 0, 63, 209, 5, 236, 19, 153, 0, 6, 65, 80, 0, 0, 14, 224, 185, 92, 64, 64, 0, 0, 14, 224, 185, 92, 65, 0, 0, 0,0,0,0,0, 8, 1, 195, 39, 0, 76, 32, 4, 2, 1, 70, 0, 1, 37, 165, 61, 202, 14, 224, 184, 148, 14, 224, 185, 92, 0, 2, 19, 152, 0, 3, 64, 160, 0, 0, 14, 224, 185, 92, 63, 128, 0, 0, 14, 224, 185, 92, 64, 64, 0, 0, 63, 209, 5, 236, 19, 153, 0, 6, 65, 80, 0, 0, 14, 224, 185, 92, 64, 64, 0, 0, 14, 224, 185, 92, 65, 0, 0, 0,0,0,0,0};

    auto bytesSent = ::send(clientSocket, createdFrame.c_str(), createdFrame.length(), MSG_NOSIGNAL);
	//send(clientSocket, array, sizeof(array),0);
    LOG_DEBUG << bytesSent << " bytes sent";
}
