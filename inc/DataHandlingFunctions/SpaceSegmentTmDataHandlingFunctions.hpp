/**
 * @file SpaceSegmentTmDataHandlingFunctions.hpp
 * @brief Functions for creating and processing TM Transfer Frames
 */

#pragma once
#include "etl/expected.h"
#include "Alert.hpp"
#include "CcsdsDefinitions.hpp"
#include "TransferFrameTM.hpp"
#include "StructureGeneration.hpp"

namespace CCSDSDataLinkLayer {
#ifdef INCLUDE_SPACE_SEGMENT_CODE
	class SpaceSegmentTmServices;

	class SpaceSegmentTmDataHandling {
		friend class SpaceSegmentTmServices;

		/**
		 * Reset the state of a Virtual Channel (also deletes the frame master copies)
		*/
		static void resetVirtualChannel(VirtualChannelSsTm& vcChan);

		/**
		 * Reset the state of a Master Channel (also deletes the frame master copies)
		 */
		static void resetMasterChannel(MasterChannelSsTm& mcChan);

		/**
		 * Serves as the main entry point from the upper layers, by storing
		 * octet synchronized and forward ordered packets along with their length so they can be later inserted to
		 * transfer frames, and transmitted.
		 *
		 *	@see p. 3.2.2 of CCSDS TM SPACE DATA LINK PROTOCOL for a definition of the 'packet' data structure
		 *	@see SANA Packet Version Number registry for a list of supported packets types.
		 *
		 *  @param packetSource Packet to be inserted. Note that the length of the packet is decided by the
		 *         length data field, not the size of the span. This means that the user can input
		 *         constant length spans, and the function will figure out the length by itself.
		 */
		static etl::expected<void, ServiceChannelNotification> storePacket(
			VirtualChannelSsTm& vcChan,
			etl::span<uint8_t> packetSource);

		/**
		 * @brief Insert a service data unit. Unlike 'Packets' its structure is not known to the Data Link (not
		 *        'Space Packets' or 'Encapsulation Packets').
		 *
		 * @param vcaSduSource Service data unit to be inserted. Note that since the sdu is of unknown structure, its
		 *                     length is assumed to be the size of the span. The function will return an error if this
		 *                     size is different than the transfer frame data field length
		 * @param packetOrderFlag User defined flag
		 * @param segmentLengthIdentifier User defined 2 bit flag (the 2 bits should be placed in the 2 least significant
		 *                                bits of the uint8_t)
		 *
		 */
		static etl::expected<void, ServiceChannelNotification> storeVcaSdu(
			const PhysicalChannel& phyChan,
			VirtualChannelSsTm& vcChan,
			etl::span<uint8_t> vcaSduSource,
			bool packetOrderFlag,
			uint8_t segmentLengthIdentifier);

		/**
		 * @brief Auxiliary function for blocking of packets stored in packet queue
		 *
		 * @param finishedOperationsFlag Signifies to vcGeneration that no more processing can take place and that it
		 *                               should exit.
		 * @param segmentationData If a packet is too large to fit in a frame, the frame pointer and the packet
		 *                         length is returned to this parameter, so that segmentation may handle this case.
		 */
		static etl::expected<void, ServiceChannelNotification> blockingTM(
			const PhysicalChannel& phyChan,
			MasterChannelSsTm& mcChan,
			VirtualChannelSsTm& vcChan,
			bool &finishedOperationsFlag,
			etl::optional<etl::pair<TransferFrameTM *, uint16_t> > &segmentationData);

		/**
		 * @brief Auxiliary function for segmentation of packets stored in packet queue
		 *
		 * @param frameTmPtr      Pointer to half full frame given by blockingTM.
		 * @param packetLength The length of the packet that is too large to fit in the frame.
		 */
		static etl::expected<void, ServiceChannelNotification> segmentationTM(
			const PhysicalChannel& phyChan,
			MasterChannelSsTm& mcChan,
			VirtualChannelSsTm& vcChan,
			TransferFrameTM *frameTmPtr,
			uint16_t packetLength);

		/**
	     * @brief Auxiliary function that generates a space packet and pushes it to the corresponding
	     * virtual channel queue.
	     *
	     * @param remainingDataFieldSpace This parameter determines the length of the generated packet. If it is
	     *                                greater or equal than the minimum space packet length (
	     *                                SpacePacketPrimaryHeaderLength + 1), then the resulting
	     *                                length is remainingDataFieldSpace. Otherwise, the minimum length packet
	     *                                will be created, which will need to be segmented across two frames.
	     *                                @see	p. 4.2.2.5 from CCSDS TM SPACE DATA LINK PROTOCOL
		 */
		static void generateIdleSpacePacket(VirtualChannelSsTm& vcChan, uint16_t remainingDataFieldSpace);

		/**
		 * Function that generates a transfer frame by combining packets via blocking and segmentation and initializing
		 * the transfer frame primary header @see p. 4.2.2 and 4.2.3 of TM Space Data Link protocol.
		 *
		 * @return void, when if there were packets to process and all the necessary frames could be created.
		 *         PACKET_QUEUE_EMPTY, if there were no packets to process. An idle frame is generated.
		 *         NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL or FRAME_LIST_FULL in case of insufficient space.
		 *
		 */
		static etl::expected<void, ServiceChannelNotification> virtualChannelGeneration(
			const PhysicalChannel &phyChan,
			MasterChannelSsTm& mcChan,
			VirtualChannelSsTm& vcChan
		);

		/**
		 * @brief Generate an OID (Only Idle Data) frames carry no information, with its data field being filled with
		 *          pseudorandom noise.
		 *
		 * @details OID frames are offered as a way to keep a constant frame rate, in the absence of frames that carry
		 *          actual information. If generated in a virtual channel with an operational control field, it can also
		 *          ensure that the CLCW flow is not interrupted
		 */
		static etl::expected<void, ServiceChannelNotification> generateOidFrame(
			const PhysicalChannel &phyChan,
			MasterChannelSsTm& mcChan,
			VirtualChannelSsTm& vcChan);

		/**
		 * @brief The Master Channel Generation Service shall be used to insert Transfer Frame
		 * Secondary Header and/or Operational Control Field service data units into Transfer Frames
		 * of a Master Channel.
		 *
		 * @note If this function attempts to process  frames with an operational control field, but it cannot
		 *       find any CLCWs available, then they will be pushed to a circular buffer, where they will wait until
		 *       a new CLCW is generated. Upon the next function call, those frames have priority. If the
		 *       CLCW shortage persists the circular buffer eventually fills up, so frames are discarded.
		 *
		 * @note Secondary header functionality in unimplemented
		 *
		 * @see p. 4.2.5 from TM Space Data Link Protocol (CCSDS 132.0-B-3)
		 */
		static etl::expected<void, ServiceChannelNotification> masterChannelGeneration(
			const PhysicalChannel &phyChan,
			MasterChannelSsTm& mcChan);

		/**
		 * The  All  Frames  Generation  Function  shall  be  used  to  perform  error  control
		 * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
		 * rate to the Channel Coding Sublayer.
		 *
		 * @param frameDestination User provided buffer to copy the fully processed transfer frame. Ensure it's length
		 *        is at least the length of a TM  transfer frame
		 *
		 * @see p. 4.2.7 from TM Space Data Link Protocol
		 */
		static etl::expected<void, ServiceChannelNotification> allFramesGeneration(
			const PhysicalChannel &phyChan,
			MasterChannelSsTm& mcChan,
			uint8_t *frameDestination);
	};
#endif // INCLUDE_SPACE_SEGMENT_CODE
} // CCSDSDataLinkLayer
