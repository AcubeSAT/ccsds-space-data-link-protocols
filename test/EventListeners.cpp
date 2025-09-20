#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#include <iostream>
#include "ChannelObjects.hpp"
#include "GroundSegmentTcServices.hpp"
#include "GroundSegmentTmServices.hpp"
#include "SpaceSegmentTcServices.hpp"
#include "SpaceSegmentTmServices.hpp"
#include "SecurityAssociationServices.hpp"

// Initialize data link once per test run
class testRunListener : public Catch::EventListenerBase {
public:
    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting(Catch::TestRunInfo const&) override {
        if (!CCSDSDataLinkLayer::Objects::initializeDataLink()) {
            std::cout << "FAILED TO INITIALIZE DATA LINK" << std::endl;
            std::exit(1);
        }
    }
};
CATCH_REGISTER_LISTENER(testRunListener)

// Reset data link once before each test case
class testCaseListener : public Catch::EventListenerBase {
public:
    using Catch::EventListenerBase::EventListenerBase;

    void testCaseStarting(Catch::TestCaseInfo const& testInfo) override {
        if (!CCSDSDataLinkLayer::GroundSegmentTcServices::resetChain(CCSDSDataLinkLayer::Generated::PhysicalChannelName::TEST_PHY).has_value() ||
            !CCSDSDataLinkLayer::SpaceSegmentTcServices::resetChain(CCSDSDataLinkLayer::Generated::PhysicalChannelName::TEST_PHY).has_value() ||
            !CCSDSDataLinkLayer::SpaceSegmentTmServices::resetChain(CCSDSDataLinkLayer::Generated::PhysicalChannelName::TEST_PHY).has_value() ||
            !CCSDSDataLinkLayer::SecurityAssociationServices::resetSequenceNumber(1).has_value()) {
            std::cout << "FAILED TO RESET DATA LINK" << std::endl;
            std::exit(1);
        }
    }
};
CATCH_REGISTER_LISTENER(testCaseListener)

