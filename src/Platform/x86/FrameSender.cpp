#include "FrameSender.hpp"

FrameSender::FrameSender() {
	socket = ::socket(AF_INET, SOCK_STREAM, 0);
	destination.sin_family = AF_INET;
	destination.sin_port = htons(port);
	inet_pton(AF_INET, "127.0.0.1", &destination.sin_addr);

	//TCP Socket Binding
	if (bind(socket, (sockaddr*) &destination, sizeof(destination))<0){
		printf("\nTCP socket binding failed\n");
	}
	else {
		LOG_DEBUG <<"Binding with 10015 finished successfully";
	}
	//TCP Socket Listening
	if(listen(socket,1) <0){
		printf("\nTCP socket listening failed\n");
	}
	else{
		LOG_DEBUG <<"Listening";
	}
	//TCP Socket Accept
	int clientSocket = accept(socket,(sockaddr*)&destination, (socklen_t*)&destination);
	if (clientSocket<0){
		printf("\nTCP socket acceptance failed\n");
	}
	else{
		LOG_DEBUG <<"Accepted";
	}
}

FrameSender::~FrameSender() {

}

void FrameSender::sendFrameToYamcs(TransferFrameTM& frame) {
	String<TmTransferFrameSize> createdFrame(frame.packetData(), TmTransferFrameSize);
	auto bytesSent = ::send(socket, createdFrame.c_str(), createdFrame.length(), MSG_NOSIGNAL);
	LOG_DEBUG << bytesSent << " bytes sent";
}