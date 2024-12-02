#include "CCSDSSecurityAssociation.hpp"

void SecurityAssociation::applySecurityTC(TransferFrameTC& frameTc, uint8_t vid, uint8_t mapid) {

    // ensure virtual channel exists and is associated with this SA
    if ((masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) ||
         (associatedChannels.find(vid) == associatedChannels.end())) {
        return;
    }
    VirtualChannel *vchan = &(masterChannel.virtualChannels.at(vid));

    // ensure MAP channel exists (if this virtual channel has map channels) and is associated with this SA
    MAPChannel *mapChannel;
    if (vchan->segmentHeaderTCPresent) {
        if (vchan->mapChannels.find(mapid) == vchan->mapChannels.end()) {
            return;
        }

        bool found = false;
        for (auto it = associatedChannels.at(vid).begin(); it != associatedChannels.at(vid).end(); ++it) {
            if (*it == mapid) {
                found = true;
            }
            break;
        }

        if (found) {
            mapChannel = &(vchan->mapChannels.at(mapid));
        }
        else {
            return;
        }
    }

    switch (saConfig) {
        case (HMAC_40_BIT):
            break;
    // add config specific applySecurity code here
    }
}

SDLSVerificationStatusCode SecurityAssociation::processSecurityTC(TransferFrameTC& frameTc, uint8_t vid, uint8_t mapid) {

    // ensure virtual channel exists and is associated with this SA
    if ((masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) ||
        (associatedChannels.find(vid) == associatedChannels.end())) {
        return INVALID_OR_UNASSOCIATED_CHANNEL;
    }
    VirtualChannel *vchan = &(masterChannel.virtualChannels.at(vid));

    // ensure MAP channel exists (if this virtual channel has map channels) and is associated with this SA
    MAPChannel *mapChannel;
    if (vchan->segmentHeaderTCPresent) {
        if (vchan->mapChannels.find(mapid) == vchan->mapChannels.end()) {
            return INVALID_OR_UNASSOCIATED_CHANNEL;
        }

        bool found = false;
        for (auto it = associatedChannels.at(vid).begin(); it != associatedChannels.at(vid).end(); ++it) {
            if (*it == mapid) {
                found = true;
            }
            break;
        }

        if (found) {
            mapChannel = &(vchan->mapChannels.at(mapid));
        }
        else {
            return INVALID_OR_UNASSOCIATED_CHANNEL;
        }
    }

    switch (saConfig) {
        case (HMAC_40_BIT):
            break;
    // add config specific processSecurity code here
    }
}