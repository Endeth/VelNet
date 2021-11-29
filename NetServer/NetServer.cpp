#include "VelNet.h"
#include "netCommon.h"

enum class PingProtocol : uint32_t //TODO move to common place for client also
{
    Ping
};

class PingServer : public Vel::Net::ServerInterface<PingProtocol>
{
public:
    PingServer( uint16_t port ) : Vel::Net::ServerInterface<PingProtocol>( port )
    {

    }

protected:
    virtual bool OnClientConnect( std::shared_ptr<Vel::Net::Connection<PingProtocol>> client ) override
    {
        return true;
    }

    virtual void OnClientDisconnect( std::shared_ptr<Vel::Net::Connection<PingProtocol>> client ) override
    {

    }
    virtual void OnMessage( std::shared_ptr<Vel::Net::Connection<PingProtocol>> client, Vel::Net::Message<PingProtocol>& msg ) override
    {
        switch( msg.header.id )
        {
            case PingProtocol::Ping:
            {
                std::cout << "[" << client->GetID() << "]: Server Ping\n";

                client->Send( msg );
            }
            break;
        }
    }
};

int main()
{
    PingServer server(13666);
    server.Start();

    while( true )
    {
        server.Update();
    }
    return 0;
}