#include <FrameOperationProcedure.h>

void FrameOperationProcedure::purge_sent_queue(){
    // TODO: Generate a a ‘Negative Confirm Response to Request to Transfer FDU’ for each stored frame
    sentQueue->clear();
}

void FrameOperationProcedure::purge_wait_queue(){
    // TODO: Generate a a ‘Negative Confirm Response to Request to Transfer FDU’ for each stored frame
    waitQueue->clear();
}

void FrameOperationProcedure::transmit_ad_frame(){
    Packet ad_frame = waitQueue->front();
    ad_frame.transferFrameSeqNumber = transmitterFrameSeqNumber;

    if (sentQueue->empty()){
        transmissionCount = 1;
    }
    ad_frame.mark_for_retransmission(0);

    waitQueue->pop_front();
    sentQueue->push_back(ad_frame);
    adOut = false;

    // @todo start the timer
    // @todo generate a 'Transmit AD Frame' request
}

void FrameOperationProcedure::transmit_bc_frame(){
    Packet bc_frame = waitQueue->front();

    bc_frame.mark_for_retransmission(0);
    transmissionCount = 1;

    // @todo start the timer
    // @todo generate a 'Transmit BC Frame' request
}

void FrameOperationProcedure::transmit_bd_frame(){
    bdOut = NOT_READY;
    Packet bd_frame = waitQueue->front();
    waitQueue->pop_front();

    // @todo generate a 'Transmit BD Frame' request
}

void FrameOperationProcedure::initiate_ad_retransmission() {
    // @todo generate an `abort` request to lower procedures
    transmissionCount = (transmissionCount == 255) ? 0 : transmissionCount + 1;
    // @todo start the timer

    for (Packet frame:*sentQueue){

        if (frame.serviceType == ServiceType::TYPE_A) {
            frame.mark_for_retransmission(1);
        }
    }
}

void FrameOperationProcedure::initiate_bc_retransmission() {
    // @todo generate an `abort` request to lower procedures
    transmissionCount = (transmissionCount == 255) ? 0 : transmissionCount + 1;
    // @todo start the timer

    for (Packet frame:*sentQueue){
        if (frame.serviceType == ServiceType::TYPE_B) {
            frame.mark_for_retransmission(1);
        }
    }
}

void FrameOperationProcedure::remove_acknowledged_frames() {
    etl::ilist<Packet>::iterator cur_frame = sentQueue->begin();

    while (cur_frame != sentQueue->end()){
        if (cur_frame->acknowledged) {
            // @todo Generate a ‘Positive Confirm Response to Request to Transfer FDU’ response
            expectedAcknowledgementSeqNumber = cur_frame->transferFrameSeqNumber;
            sentQueue->erase(cur_frame++);
        } else{
            ++cur_frame;
        }
    }
    transmissionCount = 1;
}

void FrameOperationProcedure::look_for_directive() {
    if (bcOut == FlagState::READY){
        for (Packet frame : *sentQueue) {
            if (frame.serviceType == ServiceType::TYPE_B && frame.to_be_retransmitted()) {
                bcOut == FlagState::NOT_READY;
                frame.mark_for_retransmission(0);
                // @todo Generate ‘Transmit Request for (BC) Frame’ request for this frame
            }
            transmit_bc_frame();
        }
    } else{
        // @todo? call look_for_fdu once bcOut is set to ready
    }
}

void FrameOperationProcedure::look_for_fdu() {
    if (adOut == FlagState::READY){
        for (Packet frame : *sentQueue){
            if (frame.serviceType == ServiceType::TYPE_A){
                adOut = FlagState::NOT_READY;
                frame.mark_for_retransmission(0);
                // @todo Generate ‘Transmit Request for (AD) Frame’ request for this frame
                break;
            }

            // Check if no Type-A transfer frames have been found in the sent queue
            if (adOut == FlagState::READY){
                // Search the wait queue for a suitable FDU
                // The wait queue is supposed to have a maximum capacity of one but anyway
                if (transmitterFrameSeqNumber < expectedAcknowledgementSeqNumber + fopSlidingWindow) {
                    etl::ilist<Packet>::iterator cur_frame = waitQueue->begin();
                    while (cur_frame != waitQueue->end()) {
                        if (cur_frame->serviceType == ServiceType::TYPE_A){
                            // @todo Generate ‘Accept Response to Request to Transfer FDU’
                            sentQueue->push_front(*cur_frame);
                            waitQueue->erase(cur_frame);
                        }
                    }
                }
            }

            transmit_ad_frame();
        }
    } else{
        // @todo? I think that look_for_fdu has to be automatically sent once adOut is set to ready
    }
}

void FrameOperationProcedure::initialize() {
        purge_sent_queue();
        purge_wait_queue();
        transmissionCount = 1;
        suspendState = 0;
}
