/**
 * @file MasterChannel.hpp
 */

#pragma once
#include <cstdint>
#include "ExternalContainers.hpp"
#include "TransferFrameTM.hpp"
#include "TransferFrameTC.hpp"
#include "Mutex.hpp"

namespace CCSDSDataLinkLayer {
    class SpaceSegmentTmDataHandling;
    class SpaceSegmentTcDataHandling;
    class GroundSegmentTmDataHandling;
    class GroundSegmentTcDataHandling;
    class SpaceSegmentTmServices;
    class FrameOperationProcedure;
    class FrameAcceptanceReporting;

    /**
     * Base virtual channel class containing parameters common among space and ground segment code
     */
    class MasterChannelBase {
    public:
        explicit MasterChannelBase(const Defs::Scid scid, const Defs::Pcid parentPcid)
        : channelMutex(Mutex()), scid(scid & 0x03FFU), parentPcid(parentPcid), frameCapacity(0) {}

        /**
         * @brief Protects against concurrent access to resources
         */
        Mutex channelMutex;

        [[nodiscard]] Defs::Scid getScid() const {
            return scid;
        }

        [[nodiscard]] Defs::Pcid getParentPcid() const {
            return parentPcid;
        }

        [[nodiscard]] uint16_t getFrameCapacity() const {
            return frameCapacity;
        }

        void incrementFrameCapacity(const uint16_t amount) {
            frameCapacity += amount;
        }

    protected:
        /** @brief spacecraft and master channel identifier
         *  @details 10 bits identifier for this master channel (assigned by CCSDS)
         *  @see p. 2.1.3 from CCSDS TC SPACE DATA LINK PROTOCOL
         */
        const Defs::Scid scid;

        /**
         * @brief Id of parent physical channel
         */
        const Defs::Pcid parentPcid;

        /**
         * @brief States how many frames this channel should support (used during the memory pool allocation process).
         */
        uint16_t frameCapacity;
    };

#ifdef INCLUDE_SPACE_SEGMENT_CODE
    class MasterChannelSsTm : public MasterChannelBase {
    public:
        explicit  MasterChannelSsTm(const uint16_t mcid, const Defs::Pcid parentPcid, const uint16_t ocfSduCapacity)
            : MasterChannelBase(mcid, parentPcid),
              masterChannelFrameCount(0), ocfSduCapacity(ocfSduCapacity) {}

        void initializeContainers(
            const etl::span<TransferFrameTM*>& framesAfterSecurityProcessingBuff,
            const etl::span<TransferFrameTM*>& framesAfterMcGenerationBuff,
            const etl::span<TransferFrameTM*>& waitingBufferBuff,
            const etl::span<TransferFrameTM>& frameMasterCopiesBuff,
            const etl::span<uint32_t>& frameMasterCopiesIndicesBuff,
            const etl::span<uint32_t>& ocfSduQueueBuff) {
            framesAfterSecurityProcessing = Queue(framesAfterSecurityProcessingBuff);
            framesAfterMcGeneration = Queue(framesAfterMcGenerationBuff);
            waitingBuffer = CircularBuffer(waitingBufferBuff);
            frameMasterCopies = UnorderedPool(frameMasterCopiesBuff, frameMasterCopiesIndicesBuff);
            ocfSduQueue = Queue(ocfSduQueueBuff);
        }

        [[nodiscard]] uint16_t getMasterChannelFrameCount() const {
            return masterChannelFrameCount;
        }

        void incrementMasterChannelFrameCount() {
            masterChannelFrameCount++;
        }

        void resetMasterChannelFrameCount() {
            masterChannelFrameCount = 0;
        }

        uint16_t getOcfSduCapacity() const {
            return ocfSduCapacity;
        }

        /**
         * @brief Protects frameMasterCopies and framesAfterMcGeneration from concurrent access
         */
        Mutex frameMasterCopiesAndAfterMcGenerationMutex;

        /**
         * @brief Buffer that holds pointers to TM frames that have already been processed by sdls and before mc generation
         */
        Queue<TransferFrameTM*> framesAfterSecurityProcessing;

        /**
         * @brief Buffer that holds pointers to TM frames already processed by the mc generation data handling function
         */
        Queue<TransferFrameTM*> framesAfterMcGeneration;


        /**
         * @brief Buffer that stores the actual TM transfer frame objects under this master channel
         */
        UnorderedPool<TransferFrameTM> frameMasterCopies;

        /**
         * @brief The operational control field service places the OCF_SDUs here
         */
        Queue<uint32_t> ocfSduQueue;

        /**
         * @brief Frames that have an ocf field, but no clcw could be found for them, are waiting here
         */
        CircularBuffer<TransferFrameTM*> waitingBuffer;

    private:
        /**
         * @brief A counter that keeps track the number of TM transfer frames transmitted from this master channel. The
         * master channel frame count is carried by TM transfer frames, hence the receiving side can deduce if frames
         * were lost.
         *
         * @details The initial value of this counter should be zero.
         */
        uint8_t masterChannelFrameCount;
        const uint16_t ocfSduCapacity;
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE

    // unimplemented
    // class MasterChannelGsTm {
    //
    // }

#ifdef INCLUDE_SPACE_SEGMENT_CODE
    class MasterChannelSsTc : public MasterChannelBase {
    public:
        explicit  MasterChannelSsTc(const Defs::Scid scid, const Defs::Pcid parentPcid)
             : MasterChannelBase(scid, parentPcid), noRfAvailable(false), noBitLock(false) {}

        void initializeContainers(
            const etl::span<TransferFrameTC>& frameMasterCopiesBuff,
            const etl::span<uint32_t>& frameMasterCopiesIndicesBuff) {
            frameMasterCopies = UnorderedPool(frameMasterCopiesBuff, frameMasterCopiesIndicesBuff);
        }

        [[nodiscard]] bool getNoRfAvailable() const {
            return noRfAvailable;
        }

        void setNoRfAvailable(const bool noRfAvailable) {
            this->noRfAvailable = noRfAvailable;
        }

        [[nodiscard]] bool getNoBitLock() const {
            return noBitLock;
        }

        void setNoBitLock(const bool noBitLock) {
            this->noBitLock = noBitLock;
        }

        /**
         * @brief Buffer that stores the actual TC transfer frame objects under this master channel
         */
        UnorderedPool<TransferFrameTC> frameMasterCopies;

    private:
        /**
         * @brief Used by all FARMs under this master channel to fill the "No Rf available" and "No bit lock" fields.
         *        Updated by the user, using the respective service.
         */
        bool noRfAvailable;
        bool noBitLock;
    };
#endif //INCLUDE_SPACE_SEGMENT_CODE

#ifdef INCLUDE_GROUND_SEGMENT_CODE
    class MasterChannelGsTc : public MasterChannelBase {
    public:
        explicit  MasterChannelGsTc(const Defs::Scid scid, const Defs::Pcid parentPcid)
        : MasterChannelBase(scid, parentPcid) {}

        void initializeContainers(
            const etl::span<TransferFrameTC*>& framesAfterVcGenerationBuff,
            const etl::span<TransferFrameTC>& frameMasterCopiesBuff,
            const etl::span<uint32_t>& frameMasterCopiesIndicesBuff) {
            framesAfterVcGeneration = Queue(framesAfterVcGenerationBuff);
            frameMasterCopies = UnorderedPool(frameMasterCopiesBuff, frameMasterCopiesIndicesBuff);
        }

        /**
         * @brief Buffer that holds pointers to TC frames already processed by the vc generation data handling function
         */
        Queue<TransferFrameTC*> framesAfterVcGeneration;

        /**
         * @brief Buffer that stores the actual TC transfer frame objects under this master channel
         */
        UnorderedPool<TransferFrameTC> frameMasterCopies;
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE
} // namespace CCSDSDataLinkLayer
