/**
 * @file MasterChannel.hpp
 */

#pragma once
#include <cstdint>
#include "ExternalContainers.hpp"
#include "TransferFrameTM.hpp"
#include "TransferFrameTC.hpp"

namespace CCSDSDataLinkLayer {
    /**
     * Base virtual channel class containing parameters common among space and ground segment code
     */
    class MasterChannelBase {
    public:
        explicit MasterChannelBase(const uint16_t mcid, const uint8_t parentPcid)
        : mcid(mcid & 0x03FFU), parentPcid(parentPcid), frameCapacity(0) {}

        [[nodiscard]] uint16_t getMcid() const {
            return mcid;
        }

        [[nodiscard]] uint8_t getParentPcid() const {
            return parentPcid;
        }

        [[nodiscard]] uint16_t getFrameCapacity() const {
            return frameCapacity;
        }

        void incrementFrameCapacity(const uint16_t amount) {
            frameCapacity += amount;
        }

    protected:
        /** @brief master channel identifier
         *  @details 10 bits identifier for this master channel (assigned by CCSDS)
         *  @see p. 2.1.3 from CCSDS TC SPACE DATA LINK PROTOCOL
         */
        const uint16_t mcid;

        /**
         * @brief Id of parent physical channel
         */
        const uint8_t parentPcid;

        /**
         * @brief States how many frames this channel should support (used during the memory pool allocation process).
         */
        uint16_t frameCapacity;
    };

#ifdef INCLUDE_SPACE_SEGMENT_CODE
    class MasterChannelSsTm : public MasterChannelBase {
    public:
        explicit  MasterChannelSsTm(const uint16_t mcid, const uint8_t parentPcid)
            : MasterChannelBase(mcid, parentPcid),
              masterChannelFrameCount(0) {}

        void initializeContainers(const etl::span<TransferFrameTM*>& framesAfterVcGenerationBuff,
            const etl::span<TransferFrameTM*>& framesAfterMcGenerationBuff,
            const etl::span<TransferFrameTM>& frameMasterCopiesBuff) {
            framesAfterVcGeneration = Queue(framesAfterVcGenerationBuff);
            framesAfterMcGeneration = Queue(framesAfterMcGenerationBuff);
            frameMasterCopies = UnorderedPool(frameMasterCopiesBuff);
        }

    private:
        /**
         * @brief Buffer that holds pointers to TM frames already processed by the vc generation data handling function
         */
        Queue<TransferFrameTM*> framesAfterVcGeneration;

        /**
         * @brief Buffer that holds pointers to TM frames already processed by the mc generation data handling function
         */
        Queue<TransferFrameTM*> framesAfterMcGeneration;

        /**
         * @brief A counter that keeps track the number of TM transfer frames transmitted from this master channel. The
         * master channel frame count is carried by TM transfer frames, hence the receiving side can deduce if frames
         * were lost.
         *
         * @details The initial value of this counter should be zero.
         */
        uint8_t masterChannelFrameCount;

        /**
         * @brief Buffer that stores the actual TM transfer frame objects under this master channel
         */
        UnorderedPool<TransferFrameTM> frameMasterCopies;

#ifdef ENABLE_PRIVATE_MEMBER_ACCESS
    public:
        Queue<TransferFrameTM*>& getFramesAfterVcGeneration()  {
            return framesAfterVcGeneration;
        }

        Queue<TransferFrameTM*>& getFramesAfterMcGeneration()  {
            return framesAfterMcGeneration;
        }

        uint8_t getMasterChannelFrameCount {
            return masterChannelFrameCount;
        }

        UnorderedPool<TransferFrameTM>& getFrameMasterCopies() {
            return frameMasterCopies;
        }
#endif // ENABLE_PRIVATE_MEMBER_ACCESS
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE

    // unimplemented
    // class MasterChannelGsTm {
    //
    // }

#ifdef INCLUDE_SPACE_SEGMENT_CODE
    class MasterChannelSsTc : public MasterChannelBase {
    public:
        explicit  MasterChannelSsTc(const uint16_t mscid, const uint8_t parentPcid)
             : MasterChannelBase(mscid, parentPcid) {}

        void initializeContainers(const etl::span<TransferFrameTC>& frameMasterCopiesBuff) {
            frameMasterCopies = UnorderedPool(frameMasterCopiesBuff);
        }

    private:
        /**
         * @brief Buffer that stores the actual TC transfer frame objects under this master channel
         */
        UnorderedPool<TransferFrameTC> frameMasterCopies;

#ifdef ENABLE_PRIVATE_MEMBER_ACCESS
    public:
        UnorderedPool<TransferFrameTC>& getFrameMasterCopies() {
            return frameMasterCopies;
        }
#endif // ENABLE_PRIVATE_MEMBER_ACCESS
    };
#endif //INCLUDE_SPACE_SEGMENT_CODE

#ifdef INCLUDE_GROUND_SEGMENT_CODE
    class MasterChannelGsTc : public MasterChannelBase {
    public:
        explicit  MasterChannelGsTc(const uint16_t mscid, const uint8_t parentPcid)
        : MasterChannelBase(mscid, parentPcid) {}

        void initializeContainers(
            const etl::span<TransferFrameTC*>& framesAfterVcGenerationBuff,
            const etl::span<TransferFrameTC>& frameMasterCopiesBuff
            ) {
            framesAfterVcGeneration = Queue(framesAfterVcGenerationBuff);
            frameMasterCopies = UnorderedPool(frameMasterCopiesBuff);
        }
    private:
        /**
         * @brief Buffer that holds pointers to TC frames already processed by the vc generation data handling function
         */
        Queue<TransferFrameTC*> framesAfterVcGeneration;

        /**
         * @brief Buffer that stores the actual TC transfer frame objects under this master channel
         */
        UnorderedPool<TransferFrameTC> frameMasterCopies;

#ifdef ENABLE_PRIVATE_MEMBER_ACCESS
    public:
        Queue<TransferFrameTC*>& getFramesAfterVcGeneration()  {
            return framesAfterVcGeneration;
        }

        UnorderedPool<TransferFrameTC>& getFrameMasterCopies() {
            return frameMasterCopies;
        }
#endif // ENABLE_PRIVATE_MEMBER_ACCESS
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE
} // namespace CCSDSDataLinkLayer
