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
        class ClientInterface
        {
        public:
            ClientInterface() : socket( context )
            {
            }

            virtual ~ClientInterface()
            {
                Disconnect();
            }


            bool Connect( const std::string& host, const uint16_t port )
            {
                try
                {
                    asio::ip::tcp::resolver resolver( context );
                    asio::ip::tcp::resolver::results_type endpoint = resolver.resolve( host, std::to_string( port ) );

                    connection = std::make_unique<Connection<T>>( Connection<T>::Owner::Client, context, asio::ip::tcp::socket( context ), inMessages );
                    connection->ConnectToServer( endpoint );

                    contextThread = std::thread( [this]() { context.run(); } );
                    return true;
                }
                catch( std::exception &e )
                {
                    std::cerr << "Connection exception: " << e.what() << "\n";
                    return false;
                }

                return false;
            }

            void Disconnect()
            {
                if( IsConnected() )
                {
                    connection->Disconnect();
                }

                context.stop();
                if( contextThread.joinable() )
                    contextThread.join();

                connection.release();
            }

            bool IsConnected()
            {
                if( connection )
                    return connection->IsConnected();
                else
                    return false;
            }

            void Send( const Message<T>& msg )
            {
                if( IsConnected() )
                    connection->Send( msg );
            }

            ThreadSafeQueue<OwnedMessage<T>>& IncomingMessages()
            {
                return inMessages;
            }

        protected:
            asio::io_context context;
            std::thread contextThread;
            asio::ip::tcp::socket socket;
            std::unique_ptr<Connection<T>> connection;

        private:
            ThreadSafeQueue<OwnedMessage<T>> inMessages;
        };
    }
}