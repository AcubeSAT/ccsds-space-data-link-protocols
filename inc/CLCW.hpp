#include <cstdint>
#include "CCSDSChannel.hpp"
#include "TransferFrameTM.hpp"


struct CLCW {
    uint32_t clcw;

    CLCW(uint32_t operationalControlField);

    CLCW(bool controlWordType, uint8_t clcwVersion, uint8_t statusField, uint8_t copInEffect, uint8_t vcId, uint8_t spare,
         bool noRfAvailalbe, bool noBitLock, bool lockout, bool wait, bool retransmit, uint8_t farmBCounter,
         bool spare2, uint8_t reportValue);

public:

    uint32_t getClcw();

    /**
     * @brief Control Word Type is set to 0 to indicate that the Operational Control Field contains a CLCW
     * @return bit 0 of the CLCW
     * @see 4.2.2 from Telecommand Part 2
     */
    bool getControlWordType();

    /**
     * @brief Clcw Version Number is set to value "00"  to indicate the Version-1 CLCW is used.
     * @return bits 1,2 of the CLCW
     * @see 4.2.2 from Telecommand Part 2
     */
    uint8_t getClcwVersion();

    /**
	 * @brief Field status is mission-specific and not used in the CCSDS data link protocol
	 * @return Bits 3-5 of the CLCW (the Status Field).
	 * @see p. 4.2.1.4 from TC SPACE DATA LINK PROTOCOL
	 */
    uint8_t getStatusField();

    /**
	 * @brief Used to indicate the COP that is being used.
	 * @return Bits 6-7 of the CLCW (the COP in Effect parameter)
	 * @see p. 4.2.1.5 from TC SPACE DATA LINK PROTOCOL
	 */
    uint8_t getCopInEffect();

    /**
	 * @return Bits 8-13 of the CLCW (the Virtual Channel Identifier of the Virtual Channel
	 * with which this COP report is associated).
	 * @see p. 4.2.1.6 from TC SPACE DATA LINK PROTOCOL
	 */
    uint8_t getVcId();

    uint8_t getSpare();

    /**
	 * @brief The No RF Available Flag shall provide a logical indication of the ‘ready’ status
	 * of the radio frequency (RF) elements within the space link provided by the Physical Layer.
	 * @return Bit 16 of the CLCW (the No RF Available Flag).
	 * @see p. 4.2.1.8.2 from TC SPACE DATA LINK PROTOCOL
	 */
    bool getNoRfAvailable();

    /**
	 * @brief Indicates whether "bit lock" is achieved
	 * @return Bit 17 of the CLCW (the No Bit Lock Flag).
	 * @see p. 4.2.1.8.3 from TC SPACE DATA LINK PROTOCOL
	 */
    bool getNoBitLock();

    /**
	 * @brief Indicates whether FARM is in the Lockout state
	 * @return Bit 18 of the CLCW (the Lockout Flag).
	 * @see p. 4.2.1.8.4 from TC SPACE DATA LINK PROTOCOL
	 */
    bool getLockout();

    /**
	 * @brief Indicates whether the receiver doesn't accept any packets
	 * @return Bit 19 of the CLCW (the Wait Flag)
	 * @see p. 4.1.2.8.5 from TC SPACE DATA LINK PROTOCOL
	 */
    bool getWait();

    /**
	 * @brief Indicates whether there are Type-A Frames that will need to be retransmitted
	 * @return Bit 20 of the CLCW (the Retransmit Flag).
	 * @see p. 4.2.1.8.6 from TC SPACE DATA LINK PROTOCOL
	 */
    bool getRetransmit();

    /**
	 * @brief The two least significant bits of the virtual channel
	 * @return Bits 21-22 of the CLCW (the FARM-B Counter).
	 * @see p. 4.2.1.9 from TC SPACE DATA LINK PROTOCOL
	 */
    uint8_t getFarmBCounter();

    bool getSpare2();

    /**
	 * @brief Next expected transfer frame number
	 * @return Bits 24-31 of the CLCW (the Report Value).
	 * @see p. 4.2.11.1 from TC SPACE DATA LINK PROTOCOL
	 */
    uint8_t getReportValue();
};
