#include "VelNet.h"
#include "netCommon.h"

enum class CustomMsgTypes : uint32_t //TODO move to common place for client also
{
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage
};

class CustomServer : public Vel::Net::ServerInterface<CustomMsgTypes>
{
public:
    CustomServer( uint16_t port ) 
        : Vel::Net::ServerInterface<CustomMsgTypes>( port )
    {

    }

protected:
    virtual bool OnClientConnect( std::shared_ptr<Vel::Net::Connection<CustomMsgTypes>> client ) override
    {
        return true;
    }

    virtual void OnClientDisconnect( std::shared_ptr<Vel::Net::Connection<CustomMsgTypes>> client ) override
    {

    }
    virtual void OnMessage( std::shared_ptr<Vel::Net::Connection<CustomMsgTypes>> client, Vel::Net::Message<CustomMsgTypes>& msg ) override
    {
        switch( msg.header.id )
        {
            case CustomMsgTypes::ServerPing:
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
    CustomServer server(13666);
    server.Start();

    while( true )
    {
        server.Update();
    }
    return 0;
}