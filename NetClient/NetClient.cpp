#include "VelNet.h"

enum class PingProtocol : uint32_t //TODO Move to common place for server also
{
    Ping
};

class PingClient : public Vel::Net::ClientInterface<PingProtocol>
{
public:
    void HandleIncomingMessage()
    {
        auto msg = IncomingMessages().pop_front().msg;

        switch( msg.header.id )
        {
            case PingProtocol::Ping:
            {
                auto timeNow = std::chrono::system_clock::now();
                std::chrono::system_clock::time_point timeThen;
                msg >> timeThen;
                std::cout << "Ping: " << std::chrono::duration<double>( timeNow - timeThen ).count() << "\n";
            }
            break;
        }
    }

    void PingServer()
    {
        Vel::Net::Message<PingProtocol> msg;
        msg.header.id = PingProtocol::Ping;

        auto timeNow = std::chrono::system_clock::now();

        msg << timeNow;
        Send( msg );
    }
};

int main()
{
    PingClient client;
    client.Connect( "127.0.0.1", 13666 );

    bool key[3] = { false, false, false };
    bool oldKey[3] = { false, false, false };

    bool quit = false;
    while( !quit )
    {
        if( GetForegroundWindow() == GetConsoleWindow() )
        {
            using keyReturnType = decltype( GetAsyncKeyState( 'a' ) );
            constexpr int mask = 1 << ( sizeof( keyReturnType ) * 8 - 1 ); //0x8000
            key[0] = GetAsyncKeyState( '1' ) & mask;
            key[1] = GetAsyncKeyState( '2' ) & mask;
            key[2] = GetAsyncKeyState( '3' ) & mask;
        }

        if( key[0] && !oldKey[0] )
            client.PingServer();
        if( key[2] && !oldKey[2] )
            quit = true;

        for( int i = 0; i < 3; i++ )
            oldKey[i] = key[i];

        if( client.IsConnected() )
        {
            if( !client.IncomingMessages().empty() )
                client.HandleIncomingMessage();
        }
        else
        {
            std::cout << "Server Down\n";
            quit = true;
        }
    }

    return 0;
}