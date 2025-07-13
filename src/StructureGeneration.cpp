#include "StructureGeneration.hpp"

#include <utility>

namespace CCSDSDataLinkLayer {
    bool initializeChannelContainers() {
        uint32_t transferFrameTcArrayIndex = 0;
        const uint32_t transferFrameTcArrayMaxSize = TotalTransferFrameTcSlots;

        uint32_t transferFrameTcPtrArrayIndex = 0;
        const uint32_t transferFrameTcPtrArrayMaxSize = TotalTransferFrameTcPtrSlots;

        uint32_t transferFrameTmArrayIndex = 0;
        const uint32_t transferFrameTmArrayMaxSize = TotalTransferFrameTmSlots;

        uint32_t transferFrameTmPtrArrayIndex = 0;
        const uint32_t transferFrameTmPtrArrayMaxSize = TotalTransferFrameTmPtrSlots;

        uint32_t packetLengthsArrayIndex = 0;
        const uint32_t packetLengthsArrayMaxSize = TotalTypeAdPacketSlots + TotalTypeBdPacketSlots + TotalTmPacketSlots;

        uint32_t packetOctetsArrayIndex = 0;
        const uint32_t packetOctetsArrayMaxSize =
                (TotalTypeAdPacketSlots + TotalTypeBdPacketSlots + TotalTmPacketSlots) *
                DefsAndUtils::MaxExpectedSpacePacketSize;

#ifdef INCLUDE_SPACE_SEGMENT_CODE
        // VirtualChannelSsTm
        for (auto &pair: virtualChannelSsTmMap) {
            // packetLengths dequeue (uint16_t)
            // packetOctets dequeue (uint8_t)
            if (packetLengthsArrayIndex + pair.second.getPacketCapacity() > packetLengthsArrayMaxSize ||
                packetOctetsArrayIndex + pair.second.getPacketCapacity() * DefsAndUtils::MaxExpectedSpacePacketSize >
                packetOctetsArrayMaxSize) {
                return false;
            }

            pair.second.initializeContainers(
                etl::span{packetLengthsArray + packetLengthsArrayIndex, pair.second.getPacketCapacity()},
                etl::span{
                    packetOctetsArray + packetOctetsArrayIndex,
                    pair.second.getPacketCapacity() * DefsAndUtils::MaxExpectedSpacePacketSize
                }
            );

            packetLengthsArrayIndex += pair.second.getPacketCapacity();
            packetOctetsArrayIndex += pair.second.getPacketCapacity() * DefsAndUtils::MaxExpectedSpacePacketSize;

            // increment frame capacity of the relevant master channel, so at the end of the for
            // loop, we know how much space it requires
            const uint16_t key = pair.second.getParentScid();
            if (!masterChannelSsTmMap.contains(key)) {
                return false;
            }
            masterChannelSsTmMap.at(key).incrementFrameCapacity(pair.second.getFrameCapacity());
        }

        // MasterChannelSsTm
        for (auto &pair: masterChannelSsTmMap) {
            const uint16_t frameCapacity = pair.second.getFrameCapacity();

            // framesAfterVcGeneration queue (TransferFrameTM*)
            // framesAfterMcGeneration queue (TransferFrameTM*)
            // frameMasterCopies unordered pool (TransferFrameTM)
            if (transferFrameTmPtrArrayIndex + 2 * frameCapacity > transferFrameTmPtrArrayMaxSize ||
                transferFrameTmArrayIndex + frameCapacity > transferFrameTmArrayMaxSize) {
                return false;
            }

            pair.second.initializeContainers(
                etl::span{transferFrameTmPtrArray + transferFrameTmPtrArrayIndex, frameCapacity},
                etl::span{
                    transferFrameTmPtrArray + transferFrameTmPtrArrayIndex + frameCapacity, frameCapacity
                },
                etl::span{transferFrameTmArray + transferFrameTmArrayIndex, frameCapacity}
            );

            transferFrameTmPtrArrayIndex += 2 * frameCapacity;
            transferFrameTmArrayIndex += frameCapacity;
        }

        // MapChannelSs
        for (auto &pair: mapChannelSsMap) {
            // framesAfterProcessSdlsSecurity queue (TransferFrameTC*)
            if (transferFrameTcPtrArrayIndex + pair.second.getFrameCapacity() > transferFrameTcPtrArrayMaxSize) {
                return false;
            }

            pair.second.initializeContainers(etl::span{
                transferFrameTcPtrArray + transferFrameTcPtrArrayIndex, pair.second.getFrameCapacity()
            });
            transferFrameTcPtrArrayIndex += pair.second.getFrameCapacity();

            // increment frame and packet capacities of the relevant virtual channel, so at the end of the for
            // loop, we know how much space it requires
            const DefsAndUtils::VcidScidKey key = DefsAndUtils::constructVcidScidKey(pair.second.getParentVcid(), pair.second.getParentScid());
            if (!virtualChannelSsTcMap.contains(key)) {
                return false;
            }
            virtualChannelSsTcMap.at(key).incrementTypeAdPacketCapacity(pair.second.getTypeAdPacketCapacity());
            virtualChannelSsTcMap.at(key).incrementTypeBdPacketCapacity(pair.second.getTypeBdPacketCapacity());
            virtualChannelSsTcMap.at(key).incrementFrameCapacity(pair.second.getFrameCapacity());
        }

        // VirtualChannelSsTc
        for (auto &pair: virtualChannelSsTcMap) {
            // framesAfterAllFramesReception queue (TransferFrameTC*)
            // framesAfterVcReceptionTypeAD queue (TransferFrameTC*)
            // framesAfterVcReceptionTypeBD queue (TransferFrameTC*)
            // framesAfterProcessSDLSSecurityTypeAD queue (TransferFrameTC*)
            // framesAfterProcessSDLSSecurityTypeBD queue (TransferFrameTC*)
            if (transferFrameTcPtrArrayIndex + 5 * pair.second.getFrameCapacity() > transferFrameTcPtrArrayMaxSize) {
                return false;
            }

            pair.second.initializeContainers(
                etl::span{transferFrameTcPtrArray + transferFrameTcPtrArrayIndex, pair.second.getFrameCapacity()},
                etl::span{
                    transferFrameTcPtrArray + transferFrameTcPtrArrayIndex + pair.second.getFrameCapacity(),
                    pair.second.getFrameCapacity()
                },
                etl::span{
                    transferFrameTcPtrArray + transferFrameTcPtrArrayIndex + 2 * pair.second.getFrameCapacity(),
                    pair.second.getFrameCapacity()
                },
                etl::span{
                    transferFrameTcPtrArray + transferFrameTcPtrArrayIndex + 3 * pair.second.getFrameCapacity(),
                    pair.second.getFrameCapacity()
                },
                etl::span{
                    transferFrameTcPtrArray + transferFrameTcPtrArrayIndex + 4 * pair.second.getFrameCapacity(),
                    pair.second.getFrameCapacity()
                }
            );

            transferFrameTcPtrArrayIndex += 5 * pair.second.getFrameCapacity();

            // increment frame capacity of the relevant master channel, so at the end of the for
            // loop, we know how much space it requires
            const uint16_t key = pair.second.getParentScid();
            if (!masterChannelSsTcMap.contains(key)) {
                return false;
            }
            masterChannelSsTcMap.at(key).incrementFrameCapacity(pair.second.getFrameCapacity());
        }

        // MasterChannelSsTc
        for (auto &pair: masterChannelSsTcMap) {
            const uint16_t frameCapacity = pair.second.getFrameCapacity();

            // frameMasterCopies unordered pool (TransferFrameTC)
            if (transferFrameTcArrayIndex + frameCapacity > transferFrameTcArrayMaxSize) {
                return false;
            }

            pair.second.initializeContainers(
                etl::span{transferFrameTcArray + transferFrameTcArrayIndex, frameCapacity}
            );

            transferFrameTcArrayIndex += frameCapacity;
        }
#endif // INCLUDE_SPACE_SEGMENT_CODE

#ifdef INCLUDE_GROUND_SEGMENT_CODE
        // MapChannelGs
        for (auto &pair: mapChannelGsMap) {
            // packetLengthsTypeAD (uint16_t) queue
            // packetOctetsTypeAD (uint8_t) queue
            // packetLengthsTypeBD (uint16_t) queue
            // packetOctetsTypeBD (uint8_t) queue
            if (packetLengthsArrayIndex + pair.second.getTypeAdPacketCapacity() + pair.second.getTypeBdPacketCapacity()
                > packetLengthsArrayMaxSize ||
                packetOctetsArrayIndex + (pair.second.getTypeAdPacketCapacity() + pair.second.getTypeBdPacketCapacity())
                *
                DefsAndUtils::MaxExpectedSpacePacketSize > packetOctetsArrayMaxSize) {
                return false;
            }

            pair.second.initializeContainers(
                etl::span{packetLengthsArray + packetLengthsArrayIndex, pair.second.getTypeAdPacketCapacity()},
                etl::span{
                    packetOctetsArray + packetOctetsArrayIndex, pair.second.getTypeAdPacketCapacity() *
                                                                DefsAndUtils::MaxExpectedSpacePacketSize
                },
                etl::span{
                    packetLengthsArray + packetLengthsArrayIndex + pair.second.getTypeAdPacketCapacity(),
                    pair.second.getTypeBdPacketCapacity()
                },
                etl::span{
                    packetOctetsArray + packetOctetsArrayIndex + pair.second.getTypeAdPacketCapacity() *
                    DefsAndUtils::MaxExpectedSpacePacketSize,
                    pair.second.getTypeBdPacketCapacity() * DefsAndUtils::MaxExpectedSpacePacketSize
                }
            );

            packetLengthsArrayIndex += pair.second.getTypeAdPacketCapacity() + pair.second.getTypeBdPacketCapacity();
            packetOctetsArrayIndex += (pair.second.getTypeAdPacketCapacity() + pair.second.getTypeBdPacketCapacity()) *
                    DefsAndUtils::MaxExpectedSpacePacketSize;

            // increment frame and packet capacities of the relevant virtual channel, so at the end of the for
            // loop, we know how much space it requires
            const DefsAndUtils::VcidScidKey key = DefsAndUtils::constructVcidScidKey(pair.second.getParentVcid(), pair.second.getParentScid());
            if (!virtualChannelGsTcMap.contains(key)) {
                return false;
            }
            virtualChannelGsTcMap.at(key).incrementTypeAdPacketCapacity(pair.second.getTypeAdPacketCapacity());
            virtualChannelGsTcMap.at(key).incrementTypeBdPacketCapacity(pair.second.getTypeBdPacketCapacity());
            virtualChannelGsTcMap.at(key).incrementFrameCapacity(pair.second.getFrameCapacity());
        }

        // VirtualChannelGsTc
        for (auto &pair: virtualChannelGsTcMap) {
            // packetLengthsTypeAD queue (uint16_t)
            // packetOctetsTypeAD queue (uint8_t)
            // packetLengthsTypeBD queue (uint16_t)
            // packetOctetsTypeBD queue (uint8_t)
            // framesAfterPacketProcessing queue (TransferFrameTC*)
            // framesAfterApplySDLSSecurity queue (TransferFrameTC*)
            if (packetLengthsArrayIndex + (pair.second.getTypeAdPacketCapacity() + pair.second.
                                           getTypeBdPacketCapacity()) > packetLengthsArrayMaxSize ||
                packetOctetsArrayIndex + (pair.second.getTypeAdPacketCapacity() + pair.second.getTypeBdPacketCapacity())
                *
                DefsAndUtils::MaxExpectedSpacePacketSize > packetOctetsArrayMaxSize ||
                transferFrameTcPtrArrayIndex + 2 * pair.second.getFrameCapacity() > transferFrameTcPtrArrayMaxSize) {
                return false;
            }

            pair.second.initializeContainers(
                etl::span{packetLengthsArray + packetLengthsArrayIndex, pair.second.getTypeAdPacketCapacity()},
                etl::span{
                    packetOctetsArray + packetOctetsArrayIndex,
                    pair.second.getTypeAdPacketCapacity() * DefsAndUtils::MaxExpectedSpacePacketSize
                },
                etl::span{
                    packetLengthsArray + packetLengthsArrayIndex + pair.second.getTypeAdPacketCapacity(),
                    pair.second.getTypeBdPacketCapacity()
                },
                etl::span{
                    packetOctetsArray + packetOctetsArrayIndex + pair.second.getTypeAdPacketCapacity() *
                    DefsAndUtils::MaxExpectedSpacePacketSize,
                    pair.second.getTypeBdPacketCapacity() * DefsAndUtils::MaxExpectedSpacePacketSize
                },
                etl::span{transferFrameTcPtrArray + transferFrameTcPtrArrayIndex, pair.second.getFrameCapacity()},
                etl::span{
                    transferFrameTcPtrArray + transferFrameTcPtrArrayIndex + pair.second.getFrameCapacity(),
                    pair.second.getFrameCapacity()
                }
            );

            packetLengthsArrayIndex += pair.second.getTypeAdPacketCapacity() + pair.second.getTypeBdPacketCapacity();
            packetOctetsArrayIndex += (pair.second.getTypeAdPacketCapacity() + pair.second.getTypeBdPacketCapacity()) *
                    DefsAndUtils::MaxExpectedSpacePacketSize;
            transferFrameTcPtrArrayIndex += 2 * pair.second.getFrameCapacity();

            // increment frame capacity of the relevant master channel, so at the end of the for
            // loop, we know how much space it requires
            const  uint16_t key = pair.second.getParentScid();
            if (!masterChannelGsTcMap.contains(key)) {
                return false;
            }
            masterChannelGsTcMap.at(key).incrementFrameCapacity(pair.second.getFrameCapacity());
        }

        // MasterChannelGsTc
        for (auto &pair: masterChannelGsTcMap) {
            const uint16_t frameCapacity = pair.second.getFrameCapacity();
            // framesAfterVcGeneration queue (TransferFrameTC*)
            // frameMasterCopies unorderedPool (TransferFrameTC)
            if (transferFrameTcPtrArrayIndex + frameCapacity > transferFrameTcPtrArrayMaxSize ||
                transferFrameTcArrayIndex + frameCapacity > transferFrameTcArrayMaxSize) {
                return false;
            }

            pair.second.initializeContainers(
                etl::span{transferFrameTcPtrArray + transferFrameTcPtrArrayIndex, frameCapacity},
                etl::span{transferFrameTcArray + transferFrameTcArrayIndex, frameCapacity}
            );

            transferFrameTcPtrArrayIndex += frameCapacity;
            transferFrameTcArrayIndex += frameCapacity;
        }
#endif // INCLUDE_GROUND_SEGMENT_CODE
        return true;
    }
} // CCSDSDataLinkLayer
