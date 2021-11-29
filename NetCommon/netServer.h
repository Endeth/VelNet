#pragma once

#include "netCommon.h"
#include "netConnection.h"
#include "netMessage.h"
#include "netTSQueue.h"

namespace Vel
{
    namespace Net
    {
        template <typename T>
        class ServerInterface
        {
        public:
            ServerInterface( uint16_t port )
                : acceptor( asioContext, asio::ip::tcp::endpoint( asio::ip::tcp::v4(), port ) )
            {
            }

            virtual ~ServerInterface()
            {
                Stop();
            }

            bool Start()
            {
                try
                {
                    WaitForClientConnection();
                    contextThread = std::thread( [this]() { asioContext.run(); } );
                }
                catch( std::exception& e )
                {
                    std::cerr << "[SERVER] Exception: " << e.what() << "\n";
                    return false;
                }

                std::cout << "[SERVER] Started!\n";
                return true;
            }

            void Stop()
            {
                asioContext.stop();
                if( contextThread.joinable() )
                    contextThread.join();

                std::cout << "[SERVER] Stopped!\n";
            }

            // ASYNC
            void WaitForClientConnection()
            {
                acceptor.async_accept(
                    [this]( std::error_code error, asio::ip::tcp::socket socket )
                    {
                        if( !error )
                        {
                            std::cout << "[SERVER] New connection: " << socket.remote_endpoint() << "\n";

                            std::shared_ptr<Connection<T>> newConnection = std::make_shared<Connection<T>>( Connection<T>::Owner::Server, asioContext, std::move( socket ), inMessages );

                            if( OnClientConnect( newConnection ) )
                            {
                                activeConnections.push_back( std::move( newConnection ) );
                                activeConnections.back()->ConnectToClient( connectionID++ );

                                std::cout << "[" << activeConnections.back()->GetID() << "] Connection approved\n";
                            }
                            else
                            {
                                std::cout << "[-----] Connection denied\n";
                            }
                        }
                        else
                        {
                            std::cout << "[SERVER] New connection error: " << error.message() << "\n";
                        }

                        WaitForClientConnection();
                    }
                );
            }

            void MessageClient( std::shared_ptr<Connection<T>> client, const Message<T>& msg )
            {
                if( client && client->IsConnected() )
                {
                    client->Send( msg );
                }
                else
                {
                    OnClientDisconnec( client );
                    client.reset();
                    activeConnections.erase( std::remove( activeConnections.begin(), activeConnections.end(), client ), activeConnections.end() );
                }
            }

            void MessageAllClients( const Message<T>& msg, std::shared_ptr<Connection<T>> ignoreClient = nullptr )
            {
                bool unconnectedClientExists = true;

                for( auto& client : activeConnections )
                {

                    if( client && client->IsConnected() )
                    {
                        if( client != ignoreClient )
                            client->Send( msg );
                    }
                    else
                    {
                        OnClientDisconnec( client );
                        client.reset();
                        unconnectedClientExists = true;
                    }
                }

                if( unconnectedClientExists )
                    activeConnections.erase( std::remove( activeConnections.begin(), activeConnections.end(), nullptr ), activeConnections.end() );

            }

            void Update( size_t maxMessages = std::numeric_limits<size_t>::max() )
            {
                size_t msgCount = 0;
                while( msgCount < maxMessages && !inMessages.empty() )
                {
                    auto msg = inMessages.pop_front();

                    OnMessage( msg.remote, msg.msg );

                    msgCount++;
                }
            }

        protected:
            // Can cancel connection by returning false
            virtual bool OnClientConnect( std::shared_ptr<Connection<T>> client )
            {
                return false;
            }

            virtual void OnClientDisconnect( std::shared_ptr<Connection<T>> client )
            {

            }

            virtual void OnMessage( std::shared_ptr<Connection<T>> client, Message<T>& msg )
            {

            }

            ThreadSafeQueue<OwnedMessage<T>> inMessages;

            std::deque<std::shared_ptr<Connection<T>>> activeConnections;

            asio::io_context asioContext;
            std::thread contextThread;

            asio::ip::tcp::acceptor acceptor;

            uint32_t connectionID = 10000;
        };
    }
}