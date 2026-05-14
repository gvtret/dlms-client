#include "dlms/client/client_options.hpp"

#include "dlms/client/client_status.hpp"

#include <gtest/gtest.h>

namespace {

void FillSecurityOptions(dlms::client::DlmsClientOptions& options)
{
  options.securityMode =
    dlms::client::ClientSecurityMode::AuthenticatedAndEncrypted;
  options.security.invocationCounter = 1u;
  for (std::size_t i = 0u; i < 8u; ++i) {
    options.security.clientSystemTitle[i] =
      static_cast<std::uint8_t>(0x10u + i);
    options.security.serverSystemTitle[i] =
      static_cast<std::uint8_t>(0x20u + i);
  }
  for (std::size_t i = 0u; i < 16u; ++i) {
    options.security.globalUnicastEncryptionKey[i] =
      static_cast<std::uint8_t>(0x30u + i);
    options.security.authenticationKey[i] =
      static_cast<std::uint8_t>(0x80u + i);
  }
}

} // namespace

TEST(ClientOptions, DefaultsSelectWrapperTcpNoSecurity)
{
  const dlms::client::DlmsClientOptions options =
    dlms::client::DefaultDlmsClientOptions();

  EXPECT_EQ(dlms::client::ClientProfile::WrapperTcp, options.profile);
  EXPECT_EQ(dlms::client::ClientAuthenticationMode::None,
            options.authenticationMode);
  EXPECT_EQ(dlms::client::ClientSecurityMode::None, options.securityMode);
  EXPECT_STREQ("127.0.0.1", options.wrapperTcp.host);
  EXPECT_EQ(4059u, options.wrapperTcp.port);
  EXPECT_EQ(16u, options.wrapperTcp.sourceWPort);
  EXPECT_EQ(1u, options.wrapperTcp.destinationWPort);
  EXPECT_EQ(16u, options.clientSap);
  EXPECT_EQ(1u, options.serverSap);
  EXPECT_EQ(5000u, options.connectTimeoutMs);
  EXPECT_EQ(5000u, options.requestTimeoutMs);
  EXPECT_EQ(nullptr, options.lowLevelSecurity.credential);
  EXPECT_EQ(0u, options.lowLevelSecurity.credentialSize);
  EXPECT_EQ(0u, options.security.invocationCounter);
  for (std::size_t i = 0u; i < 8u; ++i) {
    EXPECT_EQ(0u, options.security.clientSystemTitle[i]);
    EXPECT_EQ(0u, options.security.serverSystemTitle[i]);
  }
  for (std::size_t i = 0u; i < 16u; ++i) {
    EXPECT_EQ(0u, options.security.globalUnicastEncryptionKey[i]);
    EXPECT_EQ(0u, options.security.authenticationKey[i]);
  }
  EXPECT_EQ(dlms::client::ClientStatus::Ok,
            dlms::client::ValidateDlmsClientOptions(options));
}

TEST(ClientOptions, ValidatesLowLevelSecurityCredential)
{
  const std::uint8_t credential[] = {'p', 'w'};
  dlms::client::DlmsClientOptions options =
    dlms::client::DefaultDlmsClientOptions();
  options.authenticationMode =
    dlms::client::ClientAuthenticationMode::LowLevelSecurity;
  options.lowLevelSecurity.credential = credential;
  options.lowLevelSecurity.credentialSize = sizeof(credential);
  EXPECT_EQ(dlms::client::ClientStatus::Ok,
            dlms::client::ValidateDlmsClientOptions(options));

  options = dlms::client::DefaultDlmsClientOptions();
  options.authenticationMode =
    dlms::client::ClientAuthenticationMode::LowLevelSecurity;
  EXPECT_EQ(dlms::client::ClientStatus::InvalidArgument,
            dlms::client::ValidateDlmsClientOptions(options));

  options = dlms::client::DefaultDlmsClientOptions();
  options.authenticationMode =
    dlms::client::ClientAuthenticationMode::LowLevelSecurity;
  options.lowLevelSecurity.credential = credential;
  options.lowLevelSecurity.credentialSize = 0u;
  EXPECT_EQ(dlms::client::ClientStatus::InvalidArgument,
            dlms::client::ValidateDlmsClientOptions(options));

  options = dlms::client::DefaultDlmsClientOptions();
  options.authenticationMode =
    dlms::client::ClientAuthenticationMode::LowLevelSecurity;
  options.lowLevelSecurity.credential = credential;
  options.lowLevelSecurity.credentialSize = 126u;
  EXPECT_EQ(dlms::client::ClientStatus::InvalidArgument,
            dlms::client::ValidateDlmsClientOptions(options));
}

TEST(ClientOptions, RejectsInvalidEndpointAndSapValues)
{
  dlms::client::DlmsClientOptions options =
    dlms::client::DefaultDlmsClientOptions();

  options.wrapperTcp.host = "";
  EXPECT_EQ(dlms::client::ClientStatus::InvalidArgument,
            dlms::client::ValidateDlmsClientOptions(options));

  options = dlms::client::DefaultDlmsClientOptions();
  options.wrapperTcp.port = 0u;
  EXPECT_EQ(dlms::client::ClientStatus::InvalidArgument,
            dlms::client::ValidateDlmsClientOptions(options));

  options = dlms::client::DefaultDlmsClientOptions();
  options.wrapperTcp.sourceWPort = 0u;
  EXPECT_EQ(dlms::client::ClientStatus::InvalidArgument,
            dlms::client::ValidateDlmsClientOptions(options));

  options = dlms::client::DefaultDlmsClientOptions();
  options.clientSap = 0u;
  EXPECT_EQ(dlms::client::ClientStatus::InvalidArgument,
            dlms::client::ValidateDlmsClientOptions(options));
}

TEST(ClientOptions, ValidatesAuthenticatedEncryptedSecurityOptions)
{
  dlms::client::DlmsClientOptions options =
    dlms::client::DefaultDlmsClientOptions();
  FillSecurityOptions(options);
  EXPECT_EQ(dlms::client::ClientStatus::Ok,
            dlms::client::ValidateDlmsClientOptions(options));

  options = dlms::client::DefaultDlmsClientOptions();
  options.securityMode =
    dlms::client::ClientSecurityMode::AuthenticatedAndEncrypted;
  EXPECT_EQ(dlms::client::ClientStatus::InvalidArgument,
            dlms::client::ValidateDlmsClientOptions(options));
}

TEST(ClientOptions, RejectsZeroTimeouts)
{
  dlms::client::DlmsClientOptions options =
    dlms::client::DefaultDlmsClientOptions();

  options.connectTimeoutMs = 0u;
  EXPECT_EQ(dlms::client::ClientStatus::InvalidArgument,
            dlms::client::ValidateDlmsClientOptions(options));

  options = dlms::client::DefaultDlmsClientOptions();
  options.requestTimeoutMs = 0u;
  EXPECT_EQ(dlms::client::ClientStatus::InvalidArgument,
            dlms::client::ValidateDlmsClientOptions(options));
}
