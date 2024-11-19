#pragma once
#include <cstdint>

struct CLCW {
	uint32_t clcw;

	CLCW(const uint32_t operationalControlField) : clcw(operationalControlField){};

	CLCW(const bool controlWordType, const uint8_t clcwVersion, const uint8_t statusField, const uint8_t copInEffect,
	     const uint8_t vcId, const uint8_t spare, const bool noRfAvailable, const bool noBitLock, const bool lockout,
	     const bool wait, const bool retransmit, const uint8_t farmBCounter, bool spare2, uint8_t reportValue)
	    : clcw(controlWordType << 31U | clcwVersion << 29U | statusField << 26U | copInEffect << 24U | vcId << 18U |
	           noRfAvailable << 15U | noBitLock << 14U | lockout << 13U | wait << 12U | retransmit << 11U |
	           farmBCounter << 9U | reportValue){};

public:
	const uint32_t getClcw() const {
		return clcw;
	}

	/**
	 * Control Word Type is set to 0 to indicate that the Operational Control Field contains a CLCW
	 * @return bit 0 of the CLCW
	 * @see 4.2.2 from Telecommand Part 2
	 */
	const bool getControlWordType() const {
		return clcw >> 31U;
	}

	/**
	 * Clcw Version Number is set to value "00"  to indicate the Version-1 CLCW is used.
	 * @return bits 1,2 of the CLCW
	 * @see 4.2.2 from Telecommand Part 2
	 */
	const uint8_t getClcwVersion() const {
		return (clcw >> 29U) & 0x3;
	}

	/**
	 * Field status is mission-specific and not used in the CCSDS data link protocol
	 * @return Bits 3-5 of the CLCW (the Status Field).
	 * @see p. 4.2.1.4 from TC SPACE DATA LINK PROTOCOL
	 */
	const uint8_t getStatusField() const {
		return (clcw >> 26U) & 0x7;
	}

	/**
	 * Used to indicate the COP that is being used.
	 * @return Bits 6-7 of the CLCW (the COP in Effect parameter)
	 * @see p. 4.2.1.5 from TC SPACE DATA LINK PROTOCOL
	 */
	const uint8_t getCopInEffect() const {
		return (clcw >> 24U) & 0x3;
	}

	/**
	 * @return Bits 8-13 of the CLCW (the Virtual Channel Identifier of the Virtual Channel
	 * with which this COP report is associated).
	 * @see p. 4.2.1.6 from TC SPACE DATA LINK PROTOCOL
	 */
	const uint8_t getVcId() const {
		return (clcw >> 18U) & 0x3F;
	}

	/**
	 * The No RF Available Flag shall provide a logical indication of the ‘ready’ status
	 * of the radio frequency (RF) elements within the space link provided by the Physical Layer.
	 * @return Bit 16 of the CLCW (the No RF Available Flag).
	 * @see p. 4.2.1.8.2 from TC SPACE DATA LINK PROTOCOL
	 */
	const bool getNoRfAvailable() const {
		return (clcw >> 15U) & 0x1;
	}

	/**
	 * Indicates whether "bit lock" is achieved
	 * @return Bit 17 of the CLCW (the No Bit Lock Flag).
	 * @see p. 4.2.1.8.3 from TC SPACE DATA LINK PROTOCOL
	 */
	const bool getNoBitLock() const {
		return (clcw >> 14U) & 0x1;
	}

	/**
	 * Indicates whether FARM is in the Lockout state
	 * @return Bit 18 of the CLCW (the Lockout Flag).
	 * @see p. 4.2.1.8.4 from TC SPACE DATA LINK PROTOCOL
	 */
	const bool getLockout() const {
		return (clcw >> 13U) & 0x1;
	}

	/**
	 * Indicates whether the receiver doesn't accept any transfer frames
	 * @return Bit 19 of the CLCW (the Wait Flag)
	 * @see p. 4.1.2.8.5 from TC SPACE DATA LINK PROTOCOL
	 */
	const bool getWait() const {
		return (clcw >> 12U) & 0x1;
	}

	/**
	 * Indicates whether there are Type-A Frames that will need to be retransmitted
	 * @return Bit 20 of the CLCW (the Retransmit Flag).
	 * @see p. 4.2.1.8.6 from TC SPACE DATA LINK PROTOCOL
	 */
	const bool getRetransmit() const {
		return (clcw >> 11U) & 0x1;
	}

	/**
	 * The two least significant bits of the virtual channel
	 * @return Bits 21-22 of the CLCW (the FARM-B Counter).
	 * @see p. 4.2.1.9 from TC SPACE DATA LINK PROTOCOL
	 */
	const uint8_t getFarmBCounter() const {
		return (clcw >> 9U) & 0x3;
	}

	/**
	 * Next expected transfer frame number
	 * @return Bits 24-31 of the CLCW (the Report Value).
	 * @see p. 4.2.11.1 from TC SPACE DATA LINK PROTOCOL
	 */
	const uint8_t getReportValue() const {
		return clcw & 0xFF;
	}
};
