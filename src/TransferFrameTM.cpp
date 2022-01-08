#include <TransferFrameTM.hpp>
#include <TransferFrame.hpp>

TransferFrameTM::TransferFrameTM(uint8_t * tranfer_frame, uint16_t transfer_frame_length, TransferFrameType t): TransferFrame(t, transfer_frame_length, tranfer_frame), hdr(tranfer_frame) {

    if (hdr.transfer_frame_secondary_header_flag() == 1) {
        secondaryHeader = &tranfer_frame[7];
    }
	firstHeaderPointer = hdr.first_header_pointer();

	data = &tranfer_frame[firstHeaderPointer];

	if(hdr.operational_control_field_flag()==1)
	{
		operationalControlField=&tranfer_frame[transfer_frame_length -6];
	}

}
