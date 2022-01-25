#include <TransferFrameTM.hpp>
#include <TransferFrame.hpp>

TransferFrameTM::TransferFrameTM(uint8_t * tranfer_frame, uint16_t transfer_frame_length, TransferFrameType t): TransferFrame(t, transfer_frame_length, tranfer_frame), hdr(tranfer_frame) {

    if (hdr.transferFrameSecondaryHeaderFlag() == 1) {
        secondaryHeader = &tranfer_frame[7];
    }
	firstHeaderPointer = hdr.firstHeaderPointer();

	data = &tranfer_frame[firstHeaderPointer];

	if(hdr.operationalControlFieldFlag()==1)
	{
		operationalControlField=&tranfer_frame[transfer_frame_length -6];
	}

}
