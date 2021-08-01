#ifndef CCSDS_TC_PACKET_H
#define CCSDS_TC_PACKET_H

#include <CCSDS_Definitions.hpp>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <etl/memory.h>
#include "Packet.h"

class PacketTC;
	enum ServiceType { TYPE_A = 0,
	               TYPE_B = 1 };

	enum FDURequestType {
		REQUEST_PENDING = 0,
		REQUEST_POSITIVE_CONFIRM = 1,
		REQUEST_NEGATIVE_CONFIRM = 2,
	};

	uint16_t calculate_crc(uint8_t* packet, uint16_t len);

	struct TransferFrameHeaderTC : public TransferFrameHeader {

	public:
		TransferFrameHeaderTC(uint8_t *pckt) : TransferFrameHeader(pckt){

		}
		/**
		 * @brief The bypass Flag determines whether the packet will bypass FARM checks
		 */
		const bool bypass_flag() const {
			return (packet_header[0] >> 5U) & 0x01;
		}

		/**
		 * @brief The control and command Flag determines whether the packet carries control commands (Type-C) or
		 * data (Type-D)
		 */
		const bool ctrl_and_cmd_flag() const {
			return (packet_header[0] >> 4U) & 0x01;
		}

		/**
		 * @brief The length of the transfer frame
		 */
		const uint16_t transfer_frame_length() const {
			return (static_cast<uint16_t>(packet_header[2]) << 8U) | (static_cast<uint16_t>(packet_header[3])) & 0x3FF;
		}
	};

	struct PacketTC {
		void setConfSignal(FDURequestType reqSignal) {
			confSignal = reqSignal;
			// TODO Maybe signal the higher procedures here instead of having them manually take care of them
		}

		// This only compares the frame sequence number of two packets both as a way to save time when comparing the
		// data fields and because this is handy when getting rid of duplicate packets. However this could result in
		// undesired behavior if we're to delete different packets that share a frame sequence number for some reason.
		// This is normally not allowed but we have to cross-check if it is compatible with FARM checks

		friend bool operator==(const PacketTC& pack1, const PacketTC& pack2) {
			pack1.transferFrameSeqNumber == pack2.transferFrameSeqNumber;
		}

		// TODO: Handle CLCWs without any ambiguities

		const uint8_t* packet_pl_data() const {
			return data;
		}

		const uint8_t control_word_type() const {
			data[0] & 0x80 >> 7U;
		}

		const uint8_t field_status() const {
			return (data[0] & 0x1C) >> 2U;
		}

		const uint8_t cop_in_effect() const {
			return data[0] & 0x03;
		}

		const uint8_t vcId() const {
			return (data[1] & 0xFC) >> 2U;
		}

		const uint8_t no_rf_avail() const {
			return (data[2] & 0x80) >> 7U;
		}

		const uint8_t no_bit_lock() const {
			return (data[2] & 0x20) >> 5U;
		}

		const uint8_t lckout() const {
			return (data[2] & 0x20) >> 5U;
		}

		const uint8_t wt() const {
			return (data[2] & 0x10) >> 4U;
		}

		const uint8_t retransmit() const {
			return (data[2] & 0x08) >> 3U;
		}

		const uint8_t farm_b_counter() const {
			return (data[2] & 0x06) >> 1U;
		}

		const uint8_t report_value() const {
			return data[3];
		}

		/**
		 * @brief Appends the CRC code (given that the corresponding Error Correction field is present in the given
		 * virtual channel)
		 */
		void append_crc();

		/**
		 * @brief Set the number of repetitions that is determined by the virtual channel
		 */
		void set_repetitions(const uint8_t repetitions) {
			reps = repetitions;
		}

		/**
		 * @brief Determines whether the packet is marked for retransmission while in the sent queue
		 */
		const bool to_be_retransmitted() const {
			return toBeRetransmitted;
		}

		void mark_for_retransmission(bool f) {
			toBeRetransmitted = f;
		}

		const TransferFrameHeaderTC transfer_frame_header() const {
			return hdr;
		}

		const uint16_t packet_length() const {
			return packetLength;
		}

		const uint8_t segmentation_header() const {
			return segHdr;
		}

		const uint8_t global_virtual_channel_id() const {
			return gvcid;
		}

		const uint8_t virtual_channel_id() const {
			return gvcid & 0x3F;
		};

		const uint8_t map_id() const {
			return mapid;
		}

		const uint16_t spacecraft_id() const {
			return sduid;
		}

		const uint8_t transfer_frame_sequence_number() const {
			return transferFrameSeqNumber;
		}

		const uint8_t transfer_frame_version_number() const {
			return transferFrameVersionNumber;
		}

		const ServiceType service_type() const {
			return serviceType;
		}

		const bool acknowledged() const {
			return ack;
		}

		const uint8_t repetitions() const {
			return reps;
		}

		uint8_t* packet_data() const {
			return packet;
		}

		// Setters are not strictly needed in this case. The are just offered as a utility functions for the VC/MAP
		// generation services when segmenting or blocking transfer frames.
		void set_segmentation_header(uint8_t seg_hdr) {
			segHdr = seg_hdr;
		}

		void set_packet_data(uint8_t* packt_data) {
			packet = packt_data;
		}

		void set_packet_length(uint16_t packt_len) {
			packetLength = packt_len;
		}

		void set_service_type(ServiceType serv_type) {
			serviceType = serv_type;
		}

		void set_acknowledgement(bool acknowledgement) {
			ack = acknowledgement;
		}

		void set_transfer_frame_sequence_number(uint8_t frame_seq_number) {
			transferFrameSeqNumber = frame_seq_number;
		}

		PacketTC(uint8_t* packet, uint16_t packet_length, uint8_t seg_hdr, uint8_t gvcid, uint8_t mapid, uint16_t sduid,
		         ServiceType service_type, bool seg_hdr_present)
		    : packet(packet), hdr(packet), packetLength(packet_length), segHdr(seg_hdr), gvcid(gvcid), mapid(mapid),
		      sduid(sduid), serviceType(service_type), transferFrameSeqNumber(0), ack(0), toBeRetransmitted(0),
		      transferFrameVersionNumber(0), data(&packet[5 + 1 * seg_hdr_present]) {}

		PacketTC(uint8_t* packet, uint16_t packet_length);

	private:
		bool toBeRetransmitted;
		// This is used by COP to signal the higher procedures
		FDURequestType confSignal;
		uint8_t* packet;
		TransferFrameHeaderTC hdr;
		uint16_t packetLength;
		uint8_t segHdr;
		uint8_t gvcid;
		uint8_t mapid;
		uint16_t sduid;
		uint8_t transferFrameVersionNumber;
		ServiceType serviceType;
		uint8_t transferFrameSeqNumber;
		bool ack;
		uint8_t reps;
		uint8_t* data;
	};

	class CLCW {
	public:
		void setConfSignal(FDURequestType reqSignal) {
			confSignal = reqSignal;
			// TODO Maybe signal the higher procedures here instead of having them manually take care of them
		}

		/**-+
		 * @brief Field status is mission-specific and not used in the CCSDS data link protocol
		 */
		const uint8_t field_status() const {
			return fieldStatus;
		}

		/**
		 * @brief COP version in effect
		 */
		const uint8_t cop_in_effect() const {
			return copInEffect;
		}

		/**
		 * @brief Virtual Channel identifier of target virtual channel
		 */
		const uint8_t vcid() const {
			return vcId;
		}

		/**
		 * @brief Indicates whether RF transmission in the physical channel is ready
		 */
		const bool no_rf_avail() const {
			return noRFAvail;
		}

		/**
		 * @brief Indicates whether "bit lock" is achieved
		 */
		const bool no_bit_lock() const {
			return noBitLock;
		}

		/**
		 * @brief Indicates whether FARM is in the Lockout state
		 */
		const bool lockout() const {
			return lckout;
		}

		/**
		 * @brief Indicates whether the receiver doesn't accept any packets
		 */
		const bool wait() const {
			return wt;
		}

		/**
		 * @brief Indicates whether there are Type-A Frames that will need to be retransmitted
		 */
		const bool retransmission() const {
			return retransmit;
		}

		/**
		 * @brief The two least significant bits of the virtual channel
		 */
		const uint8_t farm_b_counter() const {
			return farmBCounter;
		}

		/**
		 * @brief Next expected transfer frame number
		 */
		const uint8_t report_value() const {
			return reportValue;
		}

		// @todo Is this a bit risky, ugly and hacky? Yes, like everything in life. However, I still prefer this over
		//  passing etl strings around but that's probably just me.

		/**
		 * @brief Control Link Words used for FARM reports
		 */
		CLCW(uint8_t* data) {
			clcw_data[0] = data[0];
			clcw_data[1] = data[1];
			clcw_data[2] = data[2];
			clcw_data[3] = data[3];

			fieldStatus = (clcw_data[0] & 0x1C) >> 2U;
			copInEffect = clcw_data[0] & 0x03;
			vcId = (clcw_data[1] & 0xFC) >> 2U;
			noRFAvail = (clcw_data[2] & 0x80) >> 7U;
			noBitLock = (clcw_data[2] & 0x40) >> 6U;
			lckout = (clcw_data[2] & 0x20) >> 5U;
			wt = (clcw_data[2] & 0x10) >> 4U;
			retransmit = (clcw_data[2] & 0x08) >> 3U;
			farmBCounter = (clcw_data[2] & 0x06) >> 1U;
			reportValue = clcw_data[3];
		}

		/**
		 * @brief Control Link Words used for FARM reports
		 */
		CLCW(const uint8_t field_status, const uint8_t cop_in_effect, const uint8_t virtual_channel,
		     const bool no_rf_avail, const bool no_bit_lock, const bool lockout, const bool wait,
		     const bool retransmission, const uint8_t farm_b_counter, const uint8_t report_value) {
			clcw_data[0] = (field_status << 2U) | cop_in_effect;
			clcw_data[1] = (virtual_channel << 2U);
			clcw_data[2] = (no_rf_avail << 7U) | (no_bit_lock << 6U) | (lockout << 5U) | (wait << 4U) |
			               (retransmission << 3U) | (farm_b_counter << 1U);
			clcw_data[3] = report_value;

			fieldStatus = field_status;
			noRFAvail = no_rf_avail;
			vcId = virtual_channel;
			copInEffect = cop_in_effect;
			lckout = lockout;
			noBitLock = no_bit_lock;
			wt = wait;
			retransmit = retransmission;
			farmBCounter = farm_b_counter;
			reportValue = report_value;
		}

	private:
		uint8_t clcw_data[4];
		// This is used by COP to signal the higher procedures
		FDURequestType confSignal;
		uint8_t fieldStatus;
		uint8_t copInEffect;
		uint8_t vcId;
		bool noRFAvail;
		bool noBitLock;
		bool lckout;
		bool retransmit;
		bool wt;
		uint8_t farmBCounter;
		uint8_t reportValue;
	};
#endif // CCSDS_PACKET_H
