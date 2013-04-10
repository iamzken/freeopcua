/// @author Alexander Rykovanov 2013
/// @email rykovanov.as@gmail.com
/// @brief Test of opc ua binary handshake.
/// @license GNU LGPL
///
/// Distributed under the GNU LGPL License
/// (See accompanying file LICENSE or copy at 
/// http://www.gnu.org/licenses/lgpl.html)
///

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <opc/ua/server/server.h>
#include <opc/ua/server/addons/tcp_server_addon.h>
#include <opccore/common/addons_core/addon_manager.h>
#include <opccore/common/addons_core/dynamic_addon_factory.h>

#include <chrono>
#include <thread>


using namespace testing;

const unsigned short TestPort = 33445;


namespace 
{
  inline std::string GetTcpServerAddonPath()
  {
    if (const char* path = getenv("TCP_ADDON_PATH"))
    {
      return path;
    }
    return std::string();
  }


    class IncomingConnectionProcessorMock : public OpcUa::Server::IncomingConnectionProcessor
    {
    public:
      MOCK_METHOD1(Process, void (std::shared_ptr<OpcUa::IOChannel>));
      MOCK_METHOD1(StopProcessing, void (std::shared_ptr<OpcUa::IOChannel> clientChannel));
   };

  class EchoProcessor : public OpcUa::Server::IncomingConnectionProcessor
  {
  public:
    virtual void Process(std::shared_ptr<OpcUa::IOChannel> clientChannel)
    {
      char data[4] = {0};
      clientChannel->Receive(data, 4);
      clientChannel->Send(data, 4);
    }
    virtual void StopProcessing(std::shared_ptr<OpcUa::IOChannel> clientChannel)
    {
    }
  };

  const Common::AddonID TcpServerAddonID = "tcp";

}

TEST(TcpServerAddon, CanBeLoadedLoaded)
{
  std::shared_ptr<Common::AddonsManager> addons = Common::CreateAddonsManager();
  addons->Register(TcpServerAddonID, Common::CreateDynamicAddonFactory(GetTcpServerAddonPath().c_str()));
  addons->Start();
  ASSERT_TRUE(static_cast<bool>(addons->GetAddon(TcpServerAddonID)));
  addons->Stop();
}


TEST(TcpServerAddon, StopProcessingWhenError)
{
  std::shared_ptr<Common::AddonsManager> addons = Common::CreateAddonsManager();
  addons->Register(TcpServerAddonID, Common::CreateDynamicAddonFactory(GetTcpServerAddonPath().c_str()));
  addons->Start();

  std::weak_ptr<OpcUa::Server::IncomingConnectionProcessor> testProcessor;
  {
    std::shared_ptr<OpcUa::Server::TcpServerAddon> tcpAddon = Common::GetAddon<OpcUa::Server::TcpServerAddon>(*addons, TcpServerAddonID);
    OpcUa::Server::TcpParameters tcpParams;
    tcpParams.Port = 1;
    std::shared_ptr<OpcUa::Server::IncomingConnectionProcessor> clientsProcessor(new EchoProcessor());
    testProcessor = clientsProcessor;
    ASSERT_TRUE(static_cast<bool>(testProcessor.lock()));
    tcpAddon->Listen(tcpParams, clientsProcessor);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  ASSERT_FALSE(static_cast<bool>(testProcessor.lock()));

  addons->Stop();
}

