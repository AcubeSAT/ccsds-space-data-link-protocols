#include <catch2/catch.hpp>
#include <CCSDSChannel.hpp>

TEST_CASE("CCSDS TC Channel Model") {
    // @todo add more test cases :)

    PhysicalChannel phy_channel = PhysicalChannel(1024, false, 12,
            1024, 220000, 20);


    MAPChannel map_channel_1 = MAPChannel(2, DataFieldContent::PACKET,
            1024, false, false, 1019);
    MAPChannel map_channel_2 = MAPChannel(3, DataFieldContent::VCA_SDU,
            1024, true, true, 1019);

    etl::map<uint8_t, std::shared_ptr<MAPChannel>, sizeof(std::shared_ptr<MAPChannel>)> map_channels = {
            {
                2, std::make_shared<MAPChannel>(map_channel_1)
            },
            {
                3, std::make_shared<MAPChannel>(map_channel_2)
            }
    };

    VirtualChannel virt_channel_1 = VirtualChannel(3, true, 1024, 20,
            true, 32, 32, map_channels);

    CHECK(virt_channel_1.mapChannels.at(2)->blocking == false);
    CHECK(virt_channel_1.mapChannels.at(3)->blocking == true);

    etl::map<uint8_t, std::shared_ptr<VirtualChannel>, sizeof(std::shared_ptr<VirtualChannel>)> virt_channels = {
            {
                    3, std::make_shared<VirtualChannel>(virt_channel_1)
            }
    };
    MasterChannel mast_channel = MasterChannel(virt_channels);

    CHECK(mast_channel.virtChannels.at(3)->VCID == 0x03);
}