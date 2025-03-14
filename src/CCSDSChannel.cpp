#include "CCSDSChannel.hpp"
#include "MemoryPool.hpp"

namespace CCSDSDataLinkLayer {
#ifdef SPACE_SEGMENT
void MasterChannelSpaceSegment::removeMasterRxTC(TransferFrameTC *frame_ptr) {
	etl::list<TransferFrameTC, MaxRxInMasterChannel>::iterator it;
	for (it = masterCopyRxTC.begin(); it != masterCopyRxTC.end(); ++it) {
		if (&it == frame_ptr) {
			masterCopyRxTC.erase(it);
			return;
		}
	}
}
void MasterChannelSpaceSegment::removeMasterTxTM(TransferFrameTM *frame_ptr) {
	etl::list<TransferFrameTM, MaxTxInMasterChannel>::iterator it;
	for (it = masterCopyTxTM.begin(); it != masterCopyTxTM.end(); ++it) {
		if (&it == frame_ptr) {
			masterCopyTxTM.erase(it);
			return;
		}
	}
}

#endif // SPACE_SEGMENT

#ifdef GROUND_SEGMENT
void MasterChannelGroundSegment::removeMasterTxTC(TransferFrameTC *frame_ptr) {
	etl::list<TransferFrameTC, MaxTxInMasterChannel>::iterator it;
	for (it = masterCopyTxTC.begin(); it != masterCopyTxTC.end(); ++it) {
		if (&it == frame_ptr) {
			masterCopyTxTC.erase(it);
			return;
		}
	}
}

// RxTM chain
//    void MasterChannelGroundSegment::removeMasterRxTM(TransferFrameTM *frame_ptr) {
//        etl::list<TransferFrameTM, MaxRxInMasterChannel>::iterator it;
//        for (it = masterCopyRxTM.begin(); it != masterCopyRxTM.end(); ++it) {
//            if (&it == frame_ptr) {
//                masterCopyRxTM.erase(it);
//                return;
//            }
//        }
//    }
#endif // GROUND_SEGMENT

} // namespace CCSDSDataLinkLayer