#include "dlms/client/client_status.hpp"

#include <gtest/gtest.h>

TEST(ClientStatus, NamesStableValues)
{
  EXPECT_STREQ("Ok",
               dlms::client::ClientStatusName(dlms::client::ClientStatus::Ok));
  EXPECT_STREQ(
    "InvalidArgument",
    dlms::client::ClientStatusName(
      dlms::client::ClientStatus::InvalidArgument));
  EXPECT_STREQ(
    "TransportOpenFailed",
    dlms::client::ClientStatusName(
      dlms::client::ClientStatus::TransportOpenFailed));
  EXPECT_STREQ(
    "UnsupportedFeature",
    dlms::client::ClientStatusName(
      dlms::client::ClientStatus::UnsupportedFeature));
  EXPECT_STREQ(
    "SecurityFailed",
    dlms::client::ClientStatusName(
      dlms::client::ClientStatus::SecurityFailed));
  EXPECT_STREQ(
    "Unknown",
    dlms::client::ClientStatusName(
      static_cast<dlms::client::ClientStatus>(255)));
}
