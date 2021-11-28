#include "VelNet.h"

enum class CustomMsgTypes : uint32_t //TODO Move to common place for server also
{
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage
};

class CustomClient : public Vel::Net::ClientInterface<CustomMsgTypes>
{
public:
    void HandleIncomingMessage()
    {
        auto msg = IncomingMessages().pop_front().msg;

        switch( msg.header.id )
        {
            case CustomMsgTypes::ServerPing:
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
        Vel::Net::Message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::ServerPing;

        // These implementations may vary between server/client
        auto timeNow = std::chrono::system_clock::now();

        msg << timeNow;
        Send( msg );
    }
};

class TestClient
{
public:
    TestClient( asio::io_context& context, const std::string& hostname ) : tcpResolver( context ), socket( context )
    {
        request = "GET / HTTP/1.1\n"
                  "Host: " + hostname + "\n"
                  "Connection: close\n\n";

        tcpResolver.async_resolve( hostname, "http", std::bind( &TestClient::OnResolve, this, std::placeholders::_1, std::placeholders::_2 ) );

    }

private:
    void OnResolve( std::error_code error, asio::ip::tcp::resolver::results_type results )
    {
        std::cout << "Resolve: " << error.message() << std::endl;
        asio::async_connect( socket, results, std::bind( &TestClient::OnConnect, this, std::placeholders::_1, std::placeholders::_2 ) );
    }

    void OnConnect( std::error_code error, const asio::ip::tcp::endpoint& endpoint )
    {
        std::cout << "Connect: " << error.message() << std::endl;
        asio::async_write( socket, asio::buffer( request ), std::bind( &TestClient::OnWrite, this, std::placeholders::_1, std::placeholders::_2 ) );
    }

    void OnWrite( std::error_code error, size_t bytesTransferred )
    {
        std::cout << "Write: " << error.message() << " Bytes transferred: " << bytesTransferred << std::endl;
        asio::async_read( socket, responseBuffer, std::bind( &TestClient::OnRead, this, std::placeholders::_1, std::placeholders::_2 ) );
    }

    void OnRead( std::error_code error, size_t bytesTransferred )
    {
        std::cout << "Read: " << error.message() << ", bytes transferred: " << bytesTransferred << "\n\n" << std::istream(&responseBuffer).rdbuf() << std::endl;

    }

    asio::ip::tcp::resolver tcpResolver;
    asio::ip::tcp::socket socket;
    asio::streambuf responseBuffer;
    std::string request;
};


int main()
{
    /*
    CustomClient cl;
    //cl.Connect( "127.0.0.1", 13666 );

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
            cl.PingServer();
        if( key[2] && !oldKey[2] )
            quit = true;

        for( int i = 0; i < 3; i++ )
            oldKey[i] = key[i];

        if( cl.IsConnected() )
        {
            if( !cl.IncomingMessages().empty() )
                cl.HandleIncomingMessage();
        }
        else
        {
            std::cout << "Server Down\n";
            quit = true;
        }
    }
    */

    asio::io_context asioContext;
    TestClient client( asioContext, "google.com" );
    asioContext.run();

    return 0;
}