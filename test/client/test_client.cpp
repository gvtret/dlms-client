#include "dlms/client/client.hpp"

#include "dlms/apdu/data.hpp"
#include "dlms/apdu/get.hpp"
#include "dlms/apdu/xdlms.hpp"
#include "dlms/association/association_client.hpp"
#include "dlms/profile/apdu_channel.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {

class FakeApduChannel : public dlms::profile::IApduChannel
{
public:
  FakeApduChannel()
    : openStatus(dlms::profile::ProfileStatus::Ok)
    , closeStatus(dlms::profile::ProfileStatus::Ok)
    , sendStatus(dlms::profile::ProfileStatus::Ok)
    , receiveStatus(dlms::profile::ProfileStatus::Ok)
    , open(false)
    , sendCalls(0)
    , receiveCalls(0)
  {
  }

  dlms::profile::ProfileStatus Open()
  {
    open = true;
    return openStatus;
  }

  dlms::profile::ProfileStatus Close()
  {
    open = false;
    return closeStatus;
  }

  bool IsOpen() const
  {
    return open;
  }

  dlms::profile::ProfileStatus SendApdu(dlms::profile::ProfileByteView apdu)
  {
    ++sendCalls;
    sent.assign(apdu.data, apdu.data + apdu.size);
    return sendStatus;
  }

  dlms::profile::ProfileStatus ReceiveApdu(std::vector<std::uint8_t>& apdu)
  {
    ++receiveCalls;
    if (receiveStatus == dlms::profile::ProfileStatus::Ok) {
      apdu = nextReceive;
    }
    return receiveStatus;
  }

  dlms::profile::ProfileStatus ReceiveApdu(
    dlms::profile::ProfileMutableBuffer output)
  {
    ++receiveCalls;
    if (receiveStatus != dlms::profile::ProfileStatus::Ok) {
      return receiveStatus;
    }
    if (output.size < nextReceive.size()) {
      return dlms::profile::ProfileStatus::OutputBufferTooSmall;
    }
    for (std::size_t i = 0; i < nextReceive.size(); ++i) {
      output.data[i] = nextReceive[i];
    }
    if (output.writtenSize != 0) {
      *output.writtenSize = nextReceive.size();
    }
    return dlms::profile::ProfileStatus::Ok;
  }

  dlms::profile::ProfileStatus openStatus;
  dlms::profile::ProfileStatus closeStatus;
  dlms::profile::ProfileStatus sendStatus;
  dlms::profile::ProfileStatus receiveStatus;
  bool open;
  int sendCalls;
  int receiveCalls;
  std::vector<std::uint8_t> sent;
  std::vector<std::uint8_t> nextReceive;
};

std::vector<std::uint8_t> MakeAareBytes()
{
  const std::uint8_t kAare[] = {
    0x61, 0x4E, 0x80, 0x02, 0x02, 0x84, 0xA1, 0x09,
    0x06, 0x07, 0x60, 0x85, 0x74, 0x05, 0x08, 0x01,
    0x01, 0xA2, 0x03, 0x02, 0x01, 0x00, 0xA3, 0x05,
    0xA1, 0x03, 0x02, 0x01, 0x0E, 0x88, 0x02, 0x07,
    0x80, 0x89, 0x07, 0x60, 0x85, 0x74, 0x05, 0x08,
    0x02, 0x02, 0xAA, 0x12, 0x80, 0x10, 0xC6, 0x69,
    0x73, 0x51, 0xFF, 0x4A, 0xEC, 0x29, 0xCD, 0xBA,
    0xAB, 0xF2, 0xFB, 0xE3, 0x46, 0x7C, 0xBE, 0x10,
    0x04, 0x0E, 0x08, 0x00, 0x06, 0x5F, 0x1F, 0x04,
    0x00, 0x40, 0x18, 0x1D, 0x02, 0x00, 0x00, 0x07};
  return std::vector<std::uint8_t>(kAare, kAare + sizeof(kAare));
}

std::vector<std::uint8_t> MakeRlreBytes()
{
  const std::uint8_t kRlre[] = {0x63, 0x00};
  return std::vector<std::uint8_t>(kRlre, kRlre + sizeof(kRlre));
}

std::vector<std::uint8_t> MakeLongUnsignedBytes(std::uint16_t value)
{
  std::vector<std::uint8_t> output;
  output.push_back(0x12u);
  output.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xFFu));
  output.push_back(static_cast<std::uint8_t>(value & 0xFFu));
  return output;
}

dlms::xdlms::CosemAttributeDescriptor MakeDescriptor()
{
  dlms::xdlms::CosemAttributeDescriptor descriptor =
    dlms::xdlms::EmptyCosemAttributeDescriptor();
  descriptor.classId = 7u;
  descriptor.instanceId = dlms::xdlms::CosemLogicalName(1, 0, 99, 1, 0, 255);
  descriptor.attributeId = 7u;
  return descriptor;
}

dlms::xdlms::CosemMethodDescriptor MakeMethodDescriptor()
{
  dlms::xdlms::CosemMethodDescriptor descriptor =
    dlms::xdlms::EmptyCosemMethodDescriptor();
  descriptor.classId = 7u;
  descriptor.instanceId = dlms::xdlms::CosemLogicalName(1, 0, 99, 1, 0, 255);
  descriptor.methodId = 1u;
  return descriptor;
}

std::vector<std::uint8_t> EncodeResponse(
  const dlms::apdu::XdlmsApdu& response)
{
  std::vector<std::uint8_t> output;
  EXPECT_EQ(dlms::apdu::ApduStatus::Ok,
            dlms::apdu::EncodeXdlmsApdu(response, output));
  return output;
}

std::vector<std::uint8_t> MakeGetResponse(std::uint8_t invokeIdAndPriority)
{
  dlms::apdu::XdlmsApdu response;
  response.kind = dlms::apdu::XdlmsApduKind::GetResponse;
  response.getResponse.invokeIdAndPriority = invokeIdAndPriority;
  response.getResponse.resultChoice = dlms::apdu::GetDataResultChoice::Data;
  response.getResponse.data.type = dlms::apdu::DlmsDataType::LongUnsigned;
  response.getResponse.data.unsignedValue = 0x2468u;
  return EncodeResponse(response);
}

void EstablishFacade(dlms::client::DlmsClient& client,
                     FakeApduChannel& channel)
{
  ASSERT_EQ(dlms::client::ClientStatus::Ok, client.Connect());
  channel.nextReceive = MakeAareBytes();
  ASSERT_EQ(dlms::client::ClientStatus::Ok, client.OpenAssociation());
}

} // namespace

TEST(DlmsClient, StartsDisconnectedAndConnectsChannel)
{
  FakeApduChannel channel;
  dlms::association::AssociationClient association(
    channel,
    dlms::association::DefaultAssociationOptions());
  dlms::client::DlmsClient client(channel, association);

  EXPECT_EQ(dlms::client::ClientState::Disconnected, client.State());
  EXPECT_FALSE(client.IsConnected());
  EXPECT_EQ(dlms::client::ClientStatus::Ok, client.Connect());
  EXPECT_EQ(dlms::client::ClientState::Connected, client.State());
  EXPECT_TRUE(client.IsConnected());
  EXPECT_FALSE(client.IsAssociated());
  EXPECT_TRUE(channel.open);
}

TEST(DlmsClient, ConnectMapsOpenFailure)
{
  FakeApduChannel channel;
  channel.openStatus = dlms::profile::ProfileStatus::OpenFailed;
  dlms::association::AssociationClient association(
    channel,
    dlms::association::DefaultAssociationOptions());
  dlms::client::DlmsClient client(channel, association);

  EXPECT_EQ(dlms::client::ClientStatus::ChannelOpenFailed, client.Connect());
  EXPECT_EQ(dlms::client::ClientState::Disconnected, client.State());
}

TEST(DlmsClient, OpenAssociationRequiresConnectedState)
{
  FakeApduChannel channel;
  dlms::association::AssociationClient association(
    channel,
    dlms::association::DefaultAssociationOptions());
  dlms::client::DlmsClient client(channel, association);

  EXPECT_EQ(dlms::client::ClientStatus::InvalidState,
            client.OpenAssociation());
}

TEST(DlmsClient, OpensAndReleasesAssociation)
{
  FakeApduChannel channel;
  dlms::association::AssociationClient association(
    channel,
    dlms::association::DefaultAssociationOptions());
  dlms::client::DlmsClient client(channel, association);

  EstablishFacade(client, channel);
  EXPECT_EQ(dlms::client::ClientState::Associated, client.State());
  EXPECT_TRUE(client.IsAssociated());

  channel.nextReceive = MakeRlreBytes();
  EXPECT_EQ(dlms::client::ClientStatus::Ok, client.ReleaseAssociation());
  EXPECT_EQ(dlms::client::ClientState::Disconnected, client.State());
  EXPECT_FALSE(client.IsConnected());
  EXPECT_FALSE(channel.open);
}

TEST(DlmsClient, CloseIsIdempotent)
{
  FakeApduChannel channel;
  dlms::association::AssociationClient association(
    channel,
    dlms::association::DefaultAssociationOptions());
  dlms::client::DlmsClient client(channel, association);

  EXPECT_EQ(dlms::client::ClientStatus::Ok, client.Close());
  ASSERT_EQ(dlms::client::ClientStatus::Ok, client.Connect());
  EXPECT_EQ(dlms::client::ClientStatus::Ok, client.Close());
  EXPECT_EQ(dlms::client::ClientStatus::Ok, client.Close());
  EXPECT_EQ(dlms::client::ClientState::Disconnected, client.State());
}

TEST(DlmsClient, ServicesRequireAssociation)
{
  FakeApduChannel channel;
  dlms::association::AssociationClient association(
    channel,
    dlms::association::DefaultAssociationOptions());
  dlms::client::DlmsClient client(channel, association);

  std::vector<std::uint8_t> output;
  EXPECT_EQ(dlms::client::ClientStatus::NotAssociated,
            client.Get(MakeDescriptor(), output));
  EXPECT_EQ(dlms::client::ClientStatus::NotAssociated,
            client.Set(MakeDescriptor(), MakeLongUnsignedBytes(1u)));
  EXPECT_EQ(dlms::client::ClientStatus::NotAssociated,
            client.Action(
              MakeMethodDescriptor(),
              false,
              std::vector<std::uint8_t>(),
              output));
}

TEST(DlmsClient, GetForwardsToXdlmsClient)
{
  FakeApduChannel channel;
  dlms::association::AssociationClient association(
    channel,
    dlms::association::DefaultAssociationOptions());
  dlms::client::DlmsClient client(channel, association);
  EstablishFacade(client, channel);
  channel.nextReceive = MakeGetResponse(0x81u);

  std::vector<std::uint8_t> output;
  EXPECT_EQ(dlms::client::ClientStatus::Ok,
            client.Get(MakeDescriptor(), output));
  EXPECT_EQ(MakeLongUnsignedBytes(0x2468u), output);
}
