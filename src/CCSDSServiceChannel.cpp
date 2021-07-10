#include <CCSDSServiceChannel.hpp>
#include <Packet.hpp>
#include <etl/iterator.h>
#include <Alert.hpp>

ServiceChannelNotif ServiceChannel::store(uint8_t* packet, uint16_t packet_length) {
	if (masterChannel.rxMasterCopy.full()) {
		return ServiceChannelNotif::RX_IN_MC_FULL;
	}

	if (masterChannel.rxInFramesList.full()) {
		return ServiceChannelNotif::RX_IN_BUFFER_FULL;
	}

	Packet pckt = Packet(packet, packet_length);

	if (pckt.packet_length() != packet_length) {
		return ServiceChannelNotif::RX_INVALID_LENGTH;
	}

	masterChannel.rxMasterCopy.push_back(pckt);
	Packet* master_pckt = &(masterChannel.rxMasterCopy.front());
	masterChannel.rxInFramesList.push_back(master_pckt);

	return ServiceChannelNotif::NO_SERVICE_EVENT;
}

ServiceChannelNotif ServiceChannel::store(uint8_t* packet, uint16_t packet_length, uint8_t gvcid, uint8_t mapid,
                                          uint16_t sduid, ServiceType service_type) {
	uint8_t vid = gvcid & 0x3F;
	VirtualChannel* vchan = &(masterChannel.virtChannels.at(vid));
	MAPChannel* map_channel = &(vchan->mapChannels.at(mapid));

	if (map_channel->unprocessedPacketList.full()) {
		return ServiceChannelNotif::MAP_CHANNEL_FRAME_BUFFER_FULL;
	}

	Packet packet_s = Packet(packet, packet_length, 0, gvcid, mapid, sduid, service_type);

	if (service_type == ServiceType::TYPE_A) {
		packet_s.set_repetitions(vchan->repetitionTypeAFrame);
	} else if (service_type == ServiceType::TYPE_B) {
		packet_s.set_repetitions(vchan->repetitionCOPCtrl);
	}

	masterChannel.txMasterCopy.push_back(packet_s);
	map_channel->unprocessedPacketList.push_back(&(masterChannel.txMasterCopy.back()));
	return ServiceChannelNotif::NO_SERVICE_EVENT;
}

ServiceChannelNotif ServiceChannel::mapp_request(uint8_t vid, uint8_t mapid) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));
	MAPChannel* map_channel = &(virt_channel->mapChannels.at(mapid));

	if (map_channel->unprocessedPacketList.empty()) {
		return ServiceChannelNotif::NO_TX_PACKETS_TO_PROCESS;
	}

	if (virt_channel->waitQueue.full()) {
		return ServiceChannelNotif::VC_MC_FRAME_BUFFER_FULL;
	}

	Packet* packet = map_channel->unprocessedPacketList.front();

	const uint16_t max_frame_length = virt_channel->maxFrameLength;
	bool segmentation_enabled = virt_channel->segmentHeaderPresent;
	bool blocking_enabled = virt_channel->blocking;

	const uint16_t max_packet_length = max_frame_length - (tc_primary_header_size + segmentation_enabled * 1U);

	if (packet->packet_length() > max_packet_length) {
		if (segmentation_enabled) {
			// Check if there is enough space in the buffer of the virtual channel to store_out all the segments
			uint8_t tf_n =
			    (packet->packet_length() / max_packet_length) + (packet->packet_length() % max_packet_length != 0);

			if (virt_channel->waitQueue.available() >= tf_n) {
				// Break up packet
				map_channel->unprocessedPacketList.pop_front();

				// First portion
				uint16_t seg_header = mapid || 0x40;

				Packet t_packet =
				    Packet(packet->packet_data(), max_packet_length, seg_header, packet->global_virtual_channel_id(),
				           packet->map_id(), packet->spacecraft_id(), packet->service_type());
				virt_channel->store(&t_packet);

				// Middle portion
				t_packet.set_segmentation_header(mapid || 0x00);
				for (uint8_t i = 1; i < (tf_n - 1); i++) {
					t_packet.set_packet_data(&packet->packet_data()[i * max_packet_length]);
					virt_channel->store(&t_packet);
				}

				// Last portion
				t_packet.set_segmentation_header(mapid || 0x80);
				t_packet.set_packet_data(&packet->packet_data()[(tf_n - 1) * max_packet_length]);
				t_packet.set_packet_length(packet->packet_length() % max_packet_length);
				virt_channel->store(&t_packet);
			}
		} else {
			return ServiceChannelNotif::PACKET_EXCEEDS_MAX_SIZE;
		}
	} else {
		// We've already checked whether there is enough space in the buffer so we can simply remove the packet from
		// the buffer.
		map_channel->unprocessedPacketList.pop_front();

		if (blocking_enabled) {
			// See if we can block it with other packets
			// @todo There are a few things I'm unsure about regarding blocking:
			// - You need to block neighbouring packets because otherwise there's a significant increase in complexity
			// when searching for the most space-efficient combination of packets (though it is still manageable for
			// fairly small buffer size) and you'd also need a linked list instead of a double queue
			// - Neighboring packets also need to share a primary header
			// - You'll need to allocate a new chunk of memory (unless you go with data structures that don't require
			// contiguous memory but I'm also against that)

			// for now just send packet as-is
			virt_channel->store(packet);
		} else {
			if (segmentation_enabled) {
				packet->set_segmentation_header((0xc0) || (mapid && 0x3F));
			}
			virt_channel->store(packet);
		}
	}
	return ServiceChannelNotif::NO_SERVICE_EVENT;
}

#if max_received_unprocessed_tc_in_virt_buffer > 0

ServiceChannelNotif ServiceChannel::vcpp_request(uint8_t vid) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));

	if (virt_channel->unprocessedPacketList.empty()) {
		return ServiceChannelNotif::NO_TX_PACKETS_TO_PROCESS;
	}

	if (virt_channel->waitQueue.full()) {
		return ServiceChannelNotif::VC_MC_FRAME_BUFFER_FULL;
		;
	}
	Packet* packet = virt_channel->unprocessedPacketList.front();

	const uint16_t max_frame_length = virt_channel->maxFrameLength;
	bool segmentation_enabled = virt_channel->segmentHeaderPresent;
	bool blocking_enabled = virt_channel->blocking;

	const uint16_t max_packet_length = max_frame_length - (tc_primary_header_size + segmentation_enabled * 1U);

	if (packet->packet_length() > max_packet_length) {
		if (segmentation_enabled) {
			// Check if there is enough space in the buffer of the virtual channel to store_out all the segments
			uint8_t tf_n =
			    (packet->packet_length() / max_packet_length) + (packet->packet_length() % max_packet_length != 0);

			if (virt_channel->waitQueue.capacity() >= tf_n) {
				// Break up packet
				virt_channel->unprocessedPacketList.pop_front();

				// First portion
				uint16_t seg_header = 0x40;

				Packet t_packet =
				    Packet(packet->packet_data(), max_packet_length, seg_header, packet->global_virtual_channel_id(),
				           packet->map_id(), packet->spacecraft_id(), packet->service_type());
				virt_channel->store(&t_packet);

				// Middle portion
				t_packet.set_segmentation_header(0x00);
				for (uint8_t i = 1; i < (tf_n - 1); i++) {
					t_packet.set_packet_data(&packet->packet_data()[i * max_packet_length]);
					virt_channel->store(&t_packet);
				}

				// Last portion
				t_packet.set_segmentation_header(0x80);
				t_packet.set_packet_data(&packet->packet_data()[(tf_n - 1) * max_packet_length]);
				t_packet.set_packet_length(packet->packet_length() % max_packet_length);
				virt_channel->store(&t_packet);
			}
		} else {
			return ServiceChannelNotif::PACKET_EXCEEDS_MAX_SIZE;
		}
	} else {
		// We've already checked whether there is enough space in the buffer so we can simply remove the packet from
		// the buffer.
		virt_channel->unprocessedPacketList.pop_front();

		if (blocking_enabled) {
			virt_channel->store(packet);
		} else {
			if (segmentation_enabled) {
				packet->set_segmentation_header(0xc0);
			}
			virt_channel->store(packet);
		}
	}
	return ServiceChannelNotif::NO_SERVICE_EVENT;
}

#endif

ServiceChannelNotif ServiceChannel::vc_generation_request(uint8_t vid) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));
	if (virt_channel->unprocessedPacketList.empty()) {
		return ServiceChannelNotif::NO_TX_PACKETS_TO_PROCESS;
	}

	if (masterChannel.txOutFramesList.full()) {
		return ServiceChannelNotif::TX_MC_FRAME_BUFFER_FULL;
	}

	FOPDirectiveResponse err = virt_channel->fop.transfer_fdu();

	if (err == FOPDirectiveResponse::REJECT) {
		return ServiceChannelNotif::FOP_REQUEST_REJECTED;
	}

	virt_channel->unprocessedPacketList.pop_front();
	return ServiceChannelNotif::NO_SERVICE_EVENT;
}

ServiceChannelNotif ServiceChannel::all_frames_reception_request() {
	if (masterChannel.rxInFramesList.empty()) {
		return ServiceChannelNotif::NO_RX_PACKETS_TO_PROCESS;
	}

	if (masterChannel.rxToBeTransmittedFramesList.full()) {
		return ServiceChannelNotif::RX_OUT_BUFFER_FULL;
	}

	Packet* packet = masterChannel.rxInFramesList.front();

	// Frame Delimiting and Fill Removal supposedly aren't implemented here

	/*
	 * Frame validation checks
	 */

	// Check for valid TFVN
	if (packet->transfer_frame_version_number() != 0) {
		return ServiceChannelNotif::RX_INVALID_TFVN;
	}

	// Check for valid SCID
	if (packet->spacecraft_id() == spacecraft_identifier) {
		return ServiceChannelNotif::RX_INVALID_SCID;
	}

	// Packet length is checked upon storing the packet in the MC

	// If present in channel, check if CRC is valid
#if tc_error_control_field_exists
	uint16_t len = packet->packet_length() - 2;
	uint16_t crc = calculate_crc(packet->packet_data(), packet->packet_length() - 2);

	uint16_t packet_crc =
	    ((static_cast<uint16_t>(packet->packet_data()[len]) << 8) & 0xFF00) | packet->packet_data()[len + 1];
	if (crc != packet_crc) {
		return ServiceChannelNotif::RX_INVALID_CRC;
	}
#endif
	return ServiceChannelNotif::NO_SERVICE_EVENT;
}

ServiceChannelNotif ServiceChannel::all_frames_generation_request() {
	if (masterChannel.txOutFramesList.empty()) {
		return ServiceChannelNotif::NO_TX_PACKETS_TO_PROCESS;
	}

	Packet* packet = masterChannel.txOutFramesList.front();

	if (masterChannel.errorCtrlField) {
		packet->append_crc();
	}

	masterChannel.store_transmitted_out(packet);
	return ServiceChannelNotif::NO_SERVICE_EVENT;
}

ServiceChannelNotif ServiceChannel::transmit_frame(uint8_t* pack) {
	if (masterChannel.txToBeTransmittedFramesList.empty()) {
		return ServiceChannelNotif::TX_TO_BE_TRANSMITTED_FRAMES_LIST_EMPTY;
	}

	Packet* packet = masterChannel.txToBeTransmittedFramesList.front();
	packet->set_repetitions(packet->repetitions() - 1);
	if (packet->repetitions() == 0) {
		masterChannel.txToBeTransmittedFramesList.pop_front();
	}
	memcpy(pack, packet, packet->packet_length());
	return ServiceChannelNotif::NO_SERVICE_EVENT;
}

void ServiceChannel::initiate_ad_no_clcw(uint8_t vid) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));
	FDURequestType req;
	req = virt_channel->fop.initiate_ad_no_clcw();
}

void ServiceChannel::initiate_ad_clcw(uint8_t vid) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));
	FDURequestType req;
	req = virt_channel->fop.initiate_ad_clcw();
}

void ServiceChannel::initiate_ad_unlock(uint8_t vid) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));
	FDURequestType req;
	req = virt_channel->fop.initiate_ad_unlock();
}

void ServiceChannel::initiate_ad_vr(uint8_t vid, uint8_t vr) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));
	FDURequestType req;
	req = virt_channel->fop.initiate_ad_vr(vr);
}

void ServiceChannel::terminate_ad_service(uint8_t vid) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));
	FDURequestType req;
	req = virt_channel->fop.terminate_ad_service();
}

void ServiceChannel::resume_ad_service(uint8_t vid) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));
	FDURequestType req;
	req = virt_channel->fop.resume_ad_service();
}

void ServiceChannel::set_vs(uint8_t vid, uint8_t vs) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));
	virt_channel->fop.set_vs(vs);
}

void ServiceChannel::set_fop_width(uint8_t vid, uint8_t width) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));
	virt_channel->fop.set_fop_width(width);
}

void ServiceChannel::set_t1_initial(uint8_t vid, uint16_t t1_init) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));
	virt_channel->fop.set_t1_initial(t1_init);
}

void ServiceChannel::set_transmission_limit(uint8_t vid, uint8_t vr) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));
	virt_channel->fop.set_transmission_limit(vr);
}

void ServiceChannel::set_timeout_type(uint8_t vid, bool vr) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));
	virt_channel->fop.set_timeout_type(vr);
}

// todo: this may not be needed since it doesn't affect lower procedures and doesn't change the state in any way
void ServiceChannel::invalid_directive(uint8_t vid) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));
	virt_channel->fop.invalid_directive();
}

const FOPState ServiceChannel::fop_state(uint8_t vid) const {
	return masterChannel.virtChannels.at(vid).fop.state;
}

const uint16_t ServiceChannel::t1_timer(uint8_t vid) const {
	return masterChannel.virtChannels.at(vid).fop.tiInitial;
}

const uint8_t ServiceChannel::fop_sliding_window_width(uint8_t vid) const {
	return masterChannel.virtChannels.at(vid).fop.fopSlidingWindow;
}

const bool ServiceChannel::timeout_type(uint8_t vid) const {
	return masterChannel.virtChannels.at(vid).fop.timeoutType;
}

const uint8_t ServiceChannel::transmitter_frame_seq_number(uint8_t vid) const {
	return masterChannel.virtChannels.at(vid).fop.transmitterFrameSeqNumber;
}

const uint8_t ServiceChannel::expected_frame_seq_number(uint8_t vid) const {
	return masterChannel.virtChannels.at(vid).fop.expectedAcknowledgementSeqNumber;
}

std::pair<ServiceChannelNotif, const Packet*> ServiceChannel::out_packet(const uint8_t vid, const uint8_t mapid) const {
	const etl::list<Packet*, max_received_tc_in_map_channel>* mc =
	    &(masterChannel.virtChannels.at(vid).mapChannels.at(mapid).unprocessedPacketList);
	if (mc->empty()) {
		return std::pair(ServiceChannelNotif::NO_TX_PACKETS_TO_PROCESS, nullptr);
	}

	return std::pair(ServiceChannelNotif::NO_SERVICE_EVENT, mc->front());
}

std::pair<ServiceChannelNotif, const Packet*> ServiceChannel::tx_out_packet(const uint8_t vid) const {
	const etl::list<Packet*, max_received_unprocessed_tc_in_virt_buffer>* vc =
	    &(masterChannel.virtChannels.at(vid).unprocessedPacketList);
	if (vc->empty()) {
		return std::pair(ServiceChannelNotif::NO_TX_PACKETS_TO_PROCESS, nullptr);
	}

	return std::pair(ServiceChannelNotif::NO_SERVICE_EVENT, vc->front());
}

std::pair<ServiceChannelNotif, const Packet*> ServiceChannel::tx_out_packet() const {
	if (masterChannel.txMasterCopy.empty()) {
		return std::pair(ServiceChannelNotif::NO_TX_PACKETS_TO_PROCESS, nullptr);
	}
	return std::pair(ServiceChannelNotif::NO_SERVICE_EVENT, &(masterChannel.txMasterCopy.back()));
}