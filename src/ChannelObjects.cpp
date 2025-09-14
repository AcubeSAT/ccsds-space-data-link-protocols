#include "ChannelObjects.hpp"
#include "AddressingAndParsingUtilities.hpp"
#include "SecurityAssociationKeys.hpp"

namespace CCSDSDataLinkLayer::Objects {
    using namespace Generated;

    bool validateChannelConfiguration() {
        // Check 1: There is at least one TM MasterChannel and at least one TC Master channel per physical channel
        for (auto &phyChanPair: physicalChannelMap) {
            Defs::Pcid pcid = phyChanPair.second.getPcid();

#ifdef INCLUDE_SPACE_SEGMENT_CODE
            if (!etl::any_of(masterChannelSsTcMap.begin(), masterChannelSsTcMap.end(),
                             [pcid](const auto &pair) -> bool { return pair.second.getParentPcid() == pcid; })) {
                LOG_ERROR << "Physical channel with PCID " << pcid <<
                        " does not contain any Space Segment TC Master Channels";
                return false;
            }

            if (!etl::any_of(masterChannelSsTmMap.begin(), masterChannelSsTmMap.end(),
                             [pcid](const auto &pair) -> bool { return pair.second.getParentPcid() == pcid; })) {
                LOG_ERROR << "Physical channel with PCID " << pcid <<
                        " does not contain any Space Segment TM Master Channels";
                return false;
            }
#endif

#ifdef INCLUDE_GROUND_SEGMENT_CODE
            if (!etl::any_of(masterChannelGsTcMap.begin(), masterChannelGsTcMap.end(),
                             [pcid](const auto &pair) -> bool { return pair.second.getParentPcid() == pcid; })) {
                LOG_ERROR << "Physical channel with PCID " << pcid <<
                        " does not contain any Ground Segment TC Master Channels";
                return false;
            }
#endif
        }

        // Check 2: There is at least one TM (TC) Virtual Channel per TM (TC) Master channel
#ifdef INCLUDE_SPACE_SEGMENT_CODE
        for (auto &mcChanPair: masterChannelSsTcMap) {
            Defs::Scid scid = mcChanPair.second.getScid();

            if (!etl::any_of(virtualChannelSsTcMap.begin(), virtualChannelSsTcMap.end(),
                 [scid](const auto &pair) -> bool { return pair.second.getParentScid() == scid; })) {
                LOG_ERROR << "Space segment TC Master Channel with SCID " << scid <<
                        " does not contain any Space Segment TC Virtual Channels";
                return false;
            }
        }

        for (auto &mcChanPair: masterChannelSsTmMap) {
            Defs::Scid scid = mcChanPair.second.getScid();

            if (!etl::any_of(virtualChannelSsTmMap.begin(), virtualChannelSsTmMap.end(),
                 [scid](const auto &pair) -> bool { return pair.second.getParentScid() == scid; })) {
                LOG_ERROR << "Space segment TM Master Channel with SCID " << scid <<
                        " does not contain any Space Segment TM Virtual Channels";
                return false;
                 }
        }
#endif

#ifdef INCLUDE_GROUND_SEGMENT_CODE
        for (auto &mcChanPair: masterChannelGsTcMap) {
            Defs::Scid scid = mcChanPair.second.getScid();

            if (!etl::any_of(virtualChannelGsTcMap.begin(), virtualChannelGsTcMap.end(),
                 [scid](const auto &pair) -> bool { return pair.second.getParentScid() == scid; })) {
                LOG_ERROR << "Ground segment TC Master Channel with SCID " << scid <<
                        " does not contain any Ground Segment TC Virtual Channels";
                return false;
                 }
        }
#endif


        // Check 3: There is at least one MAP Channel per TC Virtual Channel with segmentation header present
#ifdef INCLUDE_SPACE_SEGMENT_CODE
        for (auto &vcChanPair: virtualChannelSsTcMap) {
            if (vcChanPair.second.getSegmentHeaderPresent()) {
                Defs::Vcid vcid = vcChanPair.second.getVcid();
                Defs::Scid scid = vcChanPair.second.getParentScid();
                if (!etl::any_of(mapChannelSsMap.begin(), mapChannelSsMap.end(),
                     [vcid, scid](const auto &pair) -> bool { return (pair.second.getParentVcid() == vcid) && (pair.second.getParentScid() == scid);})) {
                    LOG_ERROR << "Space segment TC Virtual Channel with VCID-SCID " << vcid << "-" << scid <<
                            " does not contain any Space Segment MAP Channels, despite having a segmentation header present";
                    return false;
                }
            }
        }
#endif

#ifdef INCLUDE_GROUND_SEGMENT_CODE
        for (auto &vcChanPair: virtualChannelGsTcMap) {
            if (vcChanPair.second.getSegmentHeaderPresent()) {
                Defs::Vcid vcid = vcChanPair.second.getVcid();
                Defs::Scid scid = vcChanPair.second.getParentScid();
                Defs::VcidScidKey key = constructVcidScidKey(vcid, scid);
                if (!etl::any_of(mapChannelGsMap.begin(), mapChannelGsMap.end(),
                     [vcid, scid](const auto &pair) -> bool { return (pair.second.getParentVcid() == vcid) && (pair.second.getParentScid() == scid);})) {
                    LOG_ERROR << "Ground segment TC Virtual Channel with VCID-SCID " << vcid << "-" << scid <<
                            " does not contain any Ground Segment MAP Channels, despite having a segmentation header present";
                    return false;
                }
            }
        }
#endif

        // Check 4: All Master Channels have a Physical Channel ancestor
#ifdef INCLUDE_SPACE_SEGMENT_CODE
        for (auto &mcChanPair: masterChannelSsTcMap) {
            Defs::Pcid pcid = mcChanPair.second.getParentPcid();
            if (!etl::any_of(physicalChannelMap.begin(), physicalChannelMap.end(), [pcid](const auto& pair) -> bool { return (pair.second.getPcid() == pcid);})) {
                LOG_ERROR << "Space Segment TC Master Channel with SCID: " << mcChanPair.second.getScid() <<
                    " does not belong in any Physical Channel";
                return false;
            }
        }

        for (auto &mcChanPair: masterChannelSsTmMap) {
            Defs::Pcid pcid = mcChanPair.second.getParentPcid();
            if (!etl::any_of(physicalChannelMap.begin(), physicalChannelMap.end(), [pcid](const auto& pair) -> bool { return (pair.second.getPcid() == pcid);})) {
                LOG_ERROR << "Space Segment TM Master Channel with SCID: " << mcChanPair.second.getScid() <<
                    " does not belong in any Physical Channel";
                return false;
            }
        }
#endif

#ifdef INCLUDE_GROUND_SEGMENT_CODE
        for (auto &mcChanPair: masterChannelGsTcMap) {
            Defs::Pcid pcid = mcChanPair.second.getParentPcid();
            if (!etl::any_of(physicalChannelMap.begin(), physicalChannelMap.end(), [pcid](const auto& pair) -> bool { return (pair.second.getPcid() == pcid);})) {
                LOG_ERROR << "Ground Segment TC Master Channel with SCID: " << mcChanPair.second.getScid() <<
                    " does not belong in any Physical Channel";
                return false;
            }
        }
#endif

        // Check 5: All Virtual Channels have a Master Channel ancestor
#ifdef INCLUDE_SPACE_SEGMENT_CODE
        for (auto &vcChanPair: virtualChannelSsTcMap) {
            Defs::Scid scid = vcChanPair.second.getParentScid();
            if (!etl::any_of(masterChannelSsTcMap.begin(), masterChannelSsTcMap.end(), [scid](const auto& pair) -> bool { return (pair.second.getScid() == scid);})) {
                LOG_ERROR << "Space Segment TC Virtual Channel with VCID-SCID: " << vcChanPair.second.getVcid() << "-" << scid <<
                    " does not belong in any Master Channel";
                return false;
            }
        }

        for (auto &vcChanPair: virtualChannelSsTmMap) {
            Defs::Scid scid = vcChanPair.second.getParentScid();
            if (!etl::any_of(masterChannelSsTmMap.begin(), masterChannelSsTmMap.end(), [scid](const auto& pair) -> bool { return (pair.second.getScid() == scid);})) {
                LOG_ERROR << "Space Segment TM Virtual Channel with VCID-SCID: " << vcChanPair.second.getVcid() << "-" << scid <<
                    " does not belong in any Master Channel";
                return false;
            }
        }
#endif

#ifdef INCLUDE_GROUND_SEGMENT_CODE
        for (auto &vcChanPair: virtualChannelGsTcMap) {
            Defs::Scid scid = vcChanPair.second.getParentScid();
            if (!etl::any_of(masterChannelGsTcMap.begin(), masterChannelGsTcMap.end(), [scid](const auto& pair) -> bool { return (pair.second.getScid() == scid);})) {
                LOG_ERROR << "Ground Segment TC Virtual Channel with VCID-SCID: " << vcChanPair.second.getVcid() << "-" << scid <<
                    " does not belong in any Master Channel";
                return false;
            }
        }
#endif

        // Check 6: All MAP Channels have a Virtual Channel ancestor with segmentation header present
#ifdef INCLUDE_SPACE_SEGMENT_CODE
        for (auto &mapChanPair: mapChannelSsMap) {
            Defs::Vcid vcid = mapChanPair.second.getParentVcid();
            Defs::Scid scid = mapChanPair.second.getParentScid();
            Defs::VcidScidKey key = constructVcidScidKey(vcid, scid);
            if (!etl::any_of(virtualChannelSsTcMap.begin(), virtualChannelSsTcMap.end(),
                [key](const auto& pair) -> bool { return pair.second.getSegmentHeaderPresent() && (pair.first == key);})) {
                LOG_ERROR << "Space Segment MAP Channel with VCID-SCID: " << vcid << "-" << scid <<
                    " does not belong in any Virtual Channel";
                return false;
            }
        }
#endif

#ifdef INCLUDE_GROUND_SEGMENT_CODE
        for (auto &mapChanPair: mapChannelGsMap) {
            Defs::Vcid vcid = mapChanPair.second.getParentVcid();
            Defs::Scid scid = mapChanPair.second.getParentScid();
            Defs::VcidScidKey key = constructVcidScidKey(vcid, scid);
            if (!etl::any_of(virtualChannelGsTcMap.begin(), virtualChannelGsTcMap.end(),
                [key](const auto& pair) -> bool { return pair.second.getSegmentHeaderPresent() && (pair.first == key);})) {
                LOG_ERROR << "Ground Segment MAP Channel with VCID-SCID: " << vcid << "-" << scid <<
                    " does not belong in any Virtual Channel";
                return false;
            }
        }
#endif

        // Check 7: For every Virtual Channel with COP-1 enabled, the corresponding COP-1 object exists
#ifdef INCLUDE_SPACE_SEGMENT_CODE
        for (auto& vcChanPair: virtualChannelSsTcMap) {
            if (vcChanPair.second.getCopInEffect()) {
                Defs::Vcid vcid = vcChanPair.second.getVcid();
                Defs::Scid scid = vcChanPair.second.getParentScid();
                Defs::VcidScidKey key = constructVcidScidKey(vcid, scid);
                if (!etl::any_of(farmMap.begin(), farmMap.end(),
                    [key](const auto& pair) -> bool { return pair.first == key;})) {
                    LOG_ERROR << "Space Segment TC Virtual Channel with VCID-SCID: " << vcid << "-" << scid <<
                    " has COP-1 enabled, but no FOP object found";
                    return false;
                }
            }
        }
#endif

#ifdef INCLUDE_GROUND_SEGMENT_CODE
        for (auto& vcChanPair: virtualChannelGsTcMap) {
            if (vcChanPair.second.getCopInEffect()) {
                Defs::Vcid vcid = vcChanPair.second.getVcid();
                Defs::Scid scid = vcChanPair.second.getParentScid();
                Defs::VcidScidKey key = constructVcidScidKey(vcid, scid);
                if (!etl::any_of(fopMap.begin(), fopMap.end(),
                    [key](const auto& pair) -> bool { return pair.first == key;})) {
                    LOG_ERROR << "Ground Segment TC Virtual Channel with VCID-SCID: " << vcid << "-" << scid <<
                    " has COP-1 enabled, but no FARM object found";
                    return false;
                }
            }
        }
#endif

        // Check 8: There is no Security Association with an SPI value of 0 (this value used for indicating that there is
        //          no sa association)
#ifdef INCLUDE_SPACE_SEGMENT_CODE
        if (etl::any_of(saSpaceSegmentMap.begin(), saSpaceSegmentMap.end(),
            [](const auto &pair){return pair.second.getSecurityParameterIndex() == 0;})) {
            LOG_ERROR << "Space Segment Security Association has an Security Parameter Index value of 0";
            return false;
        }
#endif

#ifdef INCLUDE_GROUND_SEGMENT_CODE
        if (etl::any_of(saGroundSegmentMap.begin(), saGroundSegmentMap.end(),
            [](const auto &pair){return pair.second.getSecurityParameterIndex() == 0;})) {
            LOG_ERROR << "Ground Segment Security Association has an Security Parameter Index value of 0";
            return false;
        }
#endif

        // Check 9: For every Virtual or Map Channel with non-zero Security Parameter index, the corresponding Security Association exists
#ifdef INCLUDE_SPACE_SEGMENT_CODE
        for (auto& vcChanPair: virtualChannelSsTcMap) {
            if (vcChanPair.second.getAssociatedSdlsSPI().has_value()) {
                Defs::Spi spi = vcChanPair.second.getAssociatedSdlsSPI().value();
                if (!etl::any_of(saSpaceSegmentMap.begin(), saSpaceSegmentMap.end(),
                    [spi](const auto& pair) -> bool { return pair.second.getSecurityParameterIndex() == spi;})) {
                    Defs::Vcid vcid = vcChanPair.second.getVcid();
                    Defs::Scid scid = vcChanPair.second.getParentScid();
                    LOG_ERROR << "Space Segment TC Virtual Channel with VCID-SCID: " << vcid << "-" << scid <<
                    " has an invalid Security Parameter Index";
                    return false;
                }
            }
        }
#endif

#ifdef INCLUDE_GROUND_SEGMENT_CODE
        for (auto& vcChanPair: virtualChannelGsTcMap) {
            if (vcChanPair.second.getAssociatedSdlsSPI().has_value()) {
                Defs::Spi spi = vcChanPair.second.getAssociatedSdlsSPI().value();
                if (!etl::any_of(saGroundSegmentMap.begin(), saGroundSegmentMap.end(),
                    [spi](const auto& pair) -> bool { return pair.second.getSecurityParameterIndex() == spi;})) {
                    Defs::Vcid vcid = vcChanPair.second.getVcid();
                    Defs::Scid scid = vcChanPair.second.getParentScid();
                    LOG_ERROR << "Ground Segment TC Virtual Channel with VCID-SCID: " << vcid << "-" << scid <<
                    " has an invalid Security Parameter Index";
                    return false;
                }
            }
        }
#endif

        // Check 10: For every Security Association with an encryption algorithm enabled, the corresponding authentication key exists
#ifdef INCLUDE_SPACE_SEGMENT_CODE
        for (auto& saPair: saSpaceSegmentMap) {
            Defs::Spi spi = saPair.second.getSecurityParameterIndex();
            if (!etl::any_of(authenticationKeyMap.begin(), authenticationKeyMap.end(),
                [spi](const auto &pair) -> bool {return pair.first == spi;})) {
                LOG_ERROR << "No authentication key found for Space Segment Security Association with SPI: " << spi;
                return false;
            }
        }
#endif

#ifdef INCLUDE_GROUND_SEGMENT_CODE
        for (auto& saPair: saGroundSegmentMap) {
            Defs::Spi spi = saPair.second.getSecurityParameterIndex();
            if (!etl::any_of(authenticationKeyMap.begin(), authenticationKeyMap.end(),
                [spi](const auto &pair) -> bool {return pair.first == spi;})) {
                LOG_ERROR << "No authentication key found for Ground Segment Security Association with SPI: " << spi;
                return false;
            }
        }
#endif

        // Check 11: If none of the virtual channels of a TM master channel use the ocf service, then the ocf sdu capacity is 0
#ifdef INCLUDE_SPACE_SEGMENT_CODE
        for (auto& mcChanPair: masterChannelSsTmMap) {
            Defs::Scid scid = mcChanPair.second.getScid();
            if (mcChanPair.second.getOcfSduCapacity() != 0) {
                if (!etl::any_of(virtualChannelSsTmMap.begin(), virtualChannelSsTmMap.end(),
                    [scid](const auto &pair) -> bool {return pair.second.getParentScid() == scid && pair.second.getOperationalControlFieldPresent();})) {
                    LOG_ERROR << "Space Segment TM Master Channel with SCID: " << mcChanPair.second.getScid() <<
                        " has non zero ocf sdu capacity specified, but none of its Virtual Channels supports the ocf sdu service";
                    return false;
                }
            }
        }
#endif

        // Check 12: The maxTcFrameLength and tmFrameLength values of every physical channel are equal or less to the maximum definitions
        for (auto &phyChanPair : physicalChannelMap) {
            if (phyChanPair.second.getMaxTcFrameLength() > Defs::MaxTcTransferFrameLength) {
                LOG_ERROR << "Physical channel with PCID: " << phyChanPair.second.getPcid() <<
                    " has a larger maximum TC frame length than the allowed one";
                return false;
            }

            if (phyChanPair.second.getTMFrameLength() > Defs::MaxTmTransferFrameLength) {
                LOG_ERROR << "Physical channel with PCID: " << phyChanPair.second.getPcid() <<
                    " has a larger TM frame length than the allowed one";
                return false;
            }
        }

        // Check 13: In Virtual Channels with no secondary header present, the secondary header length is zero
#ifdef INCLUDE_SPACE_SEGMENT_CODE
        for (auto &vcChanPair : virtualChannelSsTmMap) {
            Defs::Vcid vcid = vcChanPair.second.getVcid();
            Defs::Scid scid = vcChanPair.second.getParentScid();

            if (!vcChanPair.second.getSecondaryHeaderPresent()) {
                if (vcChanPair.second.getSecondaryHeaderLength() != 0) {
                    LOG_ERROR << "Space Segment TM Virtual Channel with VCID-SCID: " << vcid << "-" << scid <<
                        " does not have a secondary header, but the specified length is non zero";
                    return false;
                }
            } else {
                if (vcChanPair.second.getSecondaryHeaderLength() == 0) {
                    LOG_ERROR << "Space Segment TM Virtual Channel with VCID-SCID: " << vcid << "-" << scid <<
                        " has a secondary header, but the specified length is zero";
                    return false;
                }
            }
        }
#endif

        // Check 14: In Virtual Channels that contain MAP channels, the typeAdPacketCapacity, typeBdPacketCapacity are 0
#ifdef INCLUDE_SPACE_SEGMENT_CODE
        for (auto &vcChanPair : virtualChannelSsTcMap) {
            Defs::Vcid vcid = vcChanPair.second.getVcid();
            Defs::Scid scid = vcChanPair.second.getParentScid();

            if (vcChanPair.second.getSegmentHeaderPresent()) {
                if ((vcChanPair.second.getTypeAdPacketCapacity() != 0) || (vcChanPair.second.getTypeBdPacketCapacity() != 0)) {
                    LOG_ERROR << "Space Segment TM Virtual Channel with VCID-SCID: " << vcid << "-" << scid <<
                        " has MAP channels and one or both of the specified packets lengths are non zero";
                    return false;
                }
            } else {
                if ((vcChanPair.second.getTypeAdPacketCapacity() == 0) && (vcChanPair.second.getTypeBdPacketCapacity() == 0)) {
                    LOG_ERROR << "Space Segment TM Virtual Channel with VCID-SCID: " << vcid << "-" << scid <<
                        " does not have MAP channels, and both of the specified lengths are zero";
                    return false;
                }
            }
        }
#endif

#ifdef INCLUDE_GROUND_SEGMENT_CODE
        for (auto &vcChanPair : virtualChannelGsTcMap) {
            Defs::Vcid vcid = vcChanPair.second.getVcid();
            Defs::Scid scid = vcChanPair.second.getParentScid();

            if (vcChanPair.second.getSegmentHeaderPresent()) {
                if ((vcChanPair.second.getTypeAdPacketCapacity() != 0) || (vcChanPair.second.getTypeBdPacketCapacity() != 0)) {
                    LOG_ERROR << "Ground Segment TM Virtual Channel with VCID-SCID: " << vcid << "-" << scid <<
                        " has MAP channels and one or both of the specified packets lengths are non zero";
                    return false;
                }
            } else {
                if ((vcChanPair.second.getTypeAdPacketCapacity() == 0) && (vcChanPair.second.getTypeBdPacketCapacity() == 0)) {
                    LOG_ERROR << "Ground Segment TM Virtual Channel with VCID-SCID: " << vcid << "-" << scid <<
                        " does not have MAP channels, and both of the specified lengths are zero";
                    return false;
                }
            }
        }
#endif

        // Check 15: All security associations have at least an encryption or at least an authentication algorithm defined
#ifdef INCLUDE_SPACE_SEGMENT_CODE
        for (auto &saPair : saSpaceSegmentMap) {
            if ((saPair.second.getAuthenticationAlgorithm() == Defs::AuthenticationAlgorithm::NO_AUTHENTICATION) &&
                (saPair.second.getEncryptionAlgorithm() == Defs::EncryptionAlgorithm::NO_ENCRYPTION)) {
                LOG_ERROR << "Space Segment Security Association with SPI: " << saPair.first <<
                    " has supports neither authentication, nor encryption.";
                return false;
            }
        }
#endif

#ifdef INCLUDE_GROUND_SEGMENT_CODE
        for (auto &saPair : saGroundSegmentMap) {
            if ((saPair.second.getAuthenticationAlgorithm() == Defs::AuthenticationAlgorithm::NO_AUTHENTICATION) &&
                (saPair.second.getEncryptionAlgorithm() == Defs::EncryptionAlgorithm::NO_ENCRYPTION)) {
                LOG_ERROR << "Ground Segment Security Association with SPI: " << saPair.first <<
                    " has supports neither authentication, nor encryption.";
                return false;
            }
        }
#endif

        // All checks passed
        return true;
    }

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
                Defs::MaxExpectedPacketSize;

        uint32_t indicesArrayIndex = 0;
        const uint32_t indicesArrayMaxSize = TotalTransferFrameTcSlots + TotalTransferFrameTmSlots;

#ifdef INCLUDE_SPACE_SEGMENT_CODE
        // VirtualChannelSsTm
        for (auto &pair: virtualChannelSsTmMap) {
            const uint16_t frameCapacity = pair.second.getFrameCapacity();

            // packetLengths dequeue (uint16_t)
            // packetOctets dequeue (uint8_t)
            // secondaryHeaderDataFieldOctets queue (uint8_t)
            // framesAfterVcGeneration queue (TransferFrameTM*)
            // framesAfterSecurityProcessing queue (TransferFrameTM*)
            if (packetLengthsArrayIndex + pair.second.getPacketCapacity() > packetLengthsArrayMaxSize ||
                packetOctetsArrayIndex + pair.second.getPacketCapacity() * Defs::MaxExpectedPacketSize +
                (pair.second.getSecondaryHeaderLength() - Defs::TmSecondaryHeaderIdLength) * Defs::SecondaryHeaderFieldCapacity > packetOctetsArrayMaxSize ||
                transferFrameTmPtrArrayIndex + 2 * frameCapacity > transferFrameTmPtrArrayMaxSize) {
                return false;
            }

            pair.second.initializeContainers(
                etl::span{packetLengthsArray + packetLengthsArrayIndex, pair.second.getPacketCapacity()},
                etl::span{packetOctetsArray + packetOctetsArrayIndex,
                    pair.second.getPacketCapacity() * Defs::MaxExpectedPacketSize},
                    etl::span{packetOctetsArray + packetOctetsArrayIndex
                        + pair.second.getPacketCapacity() * Defs::MaxExpectedPacketSize, (pair.second.getSecondaryHeaderLength() - Defs::TmSecondaryHeaderIdLength) * Defs::SecondaryHeaderFieldCapacity},
                etl::span{transferFrameTmPtrArray + transferFrameTmPtrArrayIndex, frameCapacity},
                etl::span{transferFrameTmPtrArray + transferFrameTmPtrArrayIndex + frameCapacity, frameCapacity}
            );

            packetLengthsArrayIndex += pair.second.getPacketCapacity();
            packetOctetsArrayIndex += pair.second.getPacketCapacity() * Defs::MaxExpectedPacketSize +
                (pair.second.getSecondaryHeaderLength() - Defs::TmSecondaryHeaderIdLength) * Defs::SecondaryHeaderFieldCapacity;
            transferFrameTmPtrArrayIndex += 2 * frameCapacity;

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

            // framesAfterSecurityProcessing queue (TransferFrameTM*)
            // framesAfterMcGeneration queue (TransferFrameTM*)
            // waitingBuffer circular buffer (TransferFrameTM*)
            // frameMasterCopies unordered pool (TransferFrameTM and uint32_t)
            // ocfSduQueue (uint32_t)
            if (transferFrameTmPtrArrayIndex + 3 * frameCapacity > transferFrameTmPtrArrayMaxSize ||
                transferFrameTmArrayIndex + frameCapacity > transferFrameTmArrayMaxSize ||
                indicesArrayIndex + frameCapacity + pair.second.getOcfSduCapacity() > indicesArrayMaxSize) {
                return false;
            }

            pair.second.initializeContainers(
                etl::span{transferFrameTmPtrArray + transferFrameTmPtrArrayIndex, frameCapacity},
                etl::span{transferFrameTmPtrArray + transferFrameTmPtrArrayIndex + frameCapacity, frameCapacity},
                etl::span{transferFrameTmPtrArray + transferFrameTmPtrArrayIndex + 2 * frameCapacity, frameCapacity},
                etl::span{transferFrameTmArray + transferFrameTmArrayIndex, frameCapacity},
                etl::span{indicesArray + indicesArrayIndex, frameCapacity},
                etl::span{indicesArray + indicesArrayIndex + frameCapacity, pair.second.getOcfSduCapacity()}
            );

            transferFrameTmPtrArrayIndex += 2 * frameCapacity;
            transferFrameTmArrayIndex += frameCapacity;
            indicesArrayIndex += frameCapacity + pair.second.getOcfSduCapacity();
        }

        // MapChannelSs
        for (auto &pair: mapChannelSsMap) {
            // framesAfterProcessSdlsSecurity queue (TransferFrameTC*)
            if (transferFrameTcPtrArrayIndex + 2 * pair.second.getFrameCapacity() > transferFrameTcPtrArrayMaxSize) {
                return false;
            }

            pair.second.initializeContainers(
                etl::span{transferFrameTcPtrArray + transferFrameTcPtrArrayIndex, pair.second.getFrameCapacity()},
                etl::span{transferFrameTcPtrArray + transferFrameTcPtrArrayIndex + pair.second.getFrameCapacity(), pair.second.getFrameCapacity()});
            transferFrameTcPtrArrayIndex += 2 * pair.second.getFrameCapacity();

            // increment frame and packet capacities of the relevant virtual channel, so at the end of the for
            // loop, we know how much space it requires
            const Defs::VcidScidKey key = constructVcidScidKey(pair.second.getParentVcid(), pair.second.getParentScid());
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

            // frameMasterCopies unordered pool (TransferFrameTC and uint32_t)
            if (transferFrameTcArrayIndex + frameCapacity > transferFrameTcArrayMaxSize ||
                indicesArrayIndex + frameCapacity > indicesArrayMaxSize) {
                return false;
            }

            pair.second.initializeContainers(
                etl::span{transferFrameTcArray + transferFrameTcArrayIndex, frameCapacity},
                etl::span{indicesArray + indicesArrayIndex, frameCapacity}
            );

            transferFrameTcArrayIndex += frameCapacity;
            indicesArrayIndex += frameCapacity;
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
                Defs::MaxExpectedPacketSize > packetOctetsArrayMaxSize) {
                return false;
            }

            pair.second.initializeContainers(
                etl::span{packetLengthsArray + packetLengthsArrayIndex, pair.second.getTypeAdPacketCapacity()},
                etl::span{
                    packetOctetsArray + packetOctetsArrayIndex, pair.second.getTypeAdPacketCapacity() *
                                                                Defs::MaxExpectedPacketSize
                },
                etl::span{
                    packetLengthsArray + packetLengthsArrayIndex + pair.second.getTypeAdPacketCapacity(),
                    pair.second.getTypeBdPacketCapacity()
                },
                etl::span{
                    packetOctetsArray + packetOctetsArrayIndex + pair.second.getTypeAdPacketCapacity() *
                    Defs::MaxExpectedPacketSize,
                    pair.second.getTypeBdPacketCapacity() * Defs::MaxExpectedPacketSize
                }
            );

            packetLengthsArrayIndex += pair.second.getTypeAdPacketCapacity() + pair.second.getTypeBdPacketCapacity();
            packetOctetsArrayIndex += (pair.second.getTypeAdPacketCapacity() + pair.second.getTypeBdPacketCapacity()) *
                    Defs::MaxExpectedPacketSize;

            // increment frame and packet capacities of the relevant virtual channel, so at the end of the for
            // loop, we know how much space it requires
            const Defs::VcidScidKey key = constructVcidScidKey(pair.second.getParentVcid(), pair.second.getParentScid());
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
                Defs::MaxExpectedPacketSize > packetOctetsArrayMaxSize ||
                transferFrameTcPtrArrayIndex + 2 * pair.second.getFrameCapacity() > transferFrameTcPtrArrayMaxSize) {
                return false;
            }

            pair.second.initializeContainers(
                etl::span{packetLengthsArray + packetLengthsArrayIndex, pair.second.getTypeAdPacketCapacity()},
                etl::span{
                    packetOctetsArray + packetOctetsArrayIndex,
                    pair.second.getTypeAdPacketCapacity() * Defs::MaxExpectedPacketSize
                },
                etl::span{
                    packetLengthsArray + packetLengthsArrayIndex + pair.second.getTypeAdPacketCapacity(),
                    pair.second.getTypeBdPacketCapacity()
                },
                etl::span{
                    packetOctetsArray + packetOctetsArrayIndex + pair.second.getTypeAdPacketCapacity() *
                    Defs::MaxExpectedPacketSize,
                    pair.second.getTypeBdPacketCapacity() * Defs::MaxExpectedPacketSize
                },
                etl::span{transferFrameTcPtrArray + transferFrameTcPtrArrayIndex, pair.second.getFrameCapacity()},
                etl::span{
                    transferFrameTcPtrArray + transferFrameTcPtrArrayIndex + pair.second.getFrameCapacity(),
                    pair.second.getFrameCapacity()
                }
            );

            packetLengthsArrayIndex += pair.second.getTypeAdPacketCapacity() + pair.second.getTypeBdPacketCapacity();
            packetOctetsArrayIndex += (pair.second.getTypeAdPacketCapacity() + pair.second.getTypeBdPacketCapacity()) *
                    Defs::MaxExpectedPacketSize;
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
            // framesAfterSecurityProcessing queue (TransferFrameTC*)
            // frameMasterCopies unorderedPool (TransferFrameTC and uint32_t)
            if (transferFrameTcPtrArrayIndex + frameCapacity > transferFrameTcPtrArrayMaxSize ||
                transferFrameTcArrayIndex + frameCapacity > transferFrameTcArrayMaxSize ||
                indicesArrayIndex + frameCapacity > indicesArrayMaxSize) {
                return false;
            }

            pair.second.initializeContainers(
                etl::span{transferFrameTcPtrArray + transferFrameTcPtrArrayIndex, frameCapacity},
                etl::span{transferFrameTcArray + transferFrameTcArrayIndex, frameCapacity},
                etl::span{indicesArray + indicesArrayIndex, frameCapacity}
            );

            transferFrameTcPtrArrayIndex += frameCapacity;
            transferFrameTcArrayIndex += frameCapacity;
            indicesArrayIndex += frameCapacity;
        }
#endif // INCLUDE_GROUND_SEGMENT_CODE

        // register master channels to physical channels
#ifdef INCLUDE_SPACE_SEGMENT_CODE
        for (auto &mcChanPair: masterChannelSsTcMap) {
            for (auto &phyChanPair: physicalChannelMap) {
                if (mcChanPair.second.getParentPcid() == phyChanPair.first) {
                    phyChanPair.second.registerTcMasterChannel(mcChanPair.first);
                }
            }
        }

        for (auto &mcChanPair: masterChannelSsTmMap) {
            for (auto &phyChanPair: physicalChannelMap) {
                if (mcChanPair.second.getParentPcid() == phyChanPair.first) {
                    phyChanPair.second.registerTmMasterChannel(mcChanPair.first);
                }
            }
        }
#endif

#ifdef INCLUDE_GROUND_SEGMENT_CODE
        for (auto &mcChanPair: masterChannelGsTcMap) {
            for (auto &phyChanPair: physicalChannelMap) {
                if (mcChanPair.second.getParentPcid() == phyChanPair.first) {
                    phyChanPair.second.registerTcMasterChannel(mcChanPair.first);
                }
            }
        }

        // for (auto &mcChanPair: masterChannelGsTmMap) {
        //     for (auto &phyChanPair: physicalChannelMap) {
        //         if (mcChanPair.second.getParentPcid() == phyChanPair.first) {
        //             phyChanPair.second.registerTmMasterChannel(mcChanPair.first);
        //         }
        //     }
        // }
#endif

        // create sorted keys arrays
        auto sortChannelKeys = [](auto& sortedKeys, const auto& channelMap) {
            sortedKeys.clear();
            for (const auto& pair : channelMap) {
                sortedKeys.push_back(pair.first);
            }
            etl::sort(sortedKeys.begin(), sortedKeys.end(),
                [&channelMap](const auto& a, const auto& b) {
                    return channelMap.at(a).getPriorityWeight() > channelMap.at(b).getPriorityWeight();
                });
        };

#ifdef INCLUDE_SPACE_SEGMENT_CODE
        sortChannelKeys(virtualChannelSsTcPrioritySortedKeys, virtualChannelSsTcMap);
        sortChannelKeys(virtualChannelSsTmPrioritySortedKeys, virtualChannelSsTmMap);
        sortChannelKeys(mapChannelSsPrioritySortedKeys, mapChannelSsMap);
#endif

#ifdef INCLUDE_GROUND_SEGMENT_CODE
        sortChannelKeys(virtualChannelGsTcPrioritySortedKeys, virtualChannelGsTcMap);
        sortChannelKeys(mapChannelGsPrioritySortedKeys, mapChannelGsMap);
#endif

        return true;
    }

    bool initializeDataLink() {
        if (!validateChannelConfiguration()) {
            return false;
        }

        if (!initializeChannelContainers()) {
            return false;
        }

        return true;
    }
} // CCSDSDataLinkLayer::Objects
