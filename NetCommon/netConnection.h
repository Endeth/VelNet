#pragma once

#include "netCommon.h"
#include "netTSQueue.h"
#include "netMessage.h"

namespace Vel
{
    namespace Net
    {
        template <typename T>
        class Connection : public std::enable_shared_from_this<Connection<T>>
        {
        public:
            enum class Owner
            {
                Server,
                Client
            };

            Connection( Owner parent, asio::io_context& context, asio::ip::tcp::socket conSocket, ThreadSafeQueue<OwnedMessage<T>>& inQ )
                : inMessages( inQ ), asioContext( context), socket( std::move( conSocket ) )
            {
                ownerType = parent;
            }

            virtual ~Connection()
            {
            }

            uint32_t GetID() const
            {
                return id;
            }

            void ConnectToClient( uint32_t uid = 0 )
            {
                if( ownerType == Owner::Server )
                {
                    if( IsConnected() )
                    {
                        id = uid;
                        ReadHeader();
                    }
                }
            }

            void ConnectToServer( const asio::ip::tcp::resolver::results_type &endpoints )
            {
                if( ownerType == Owner::Client )
                {
                    asio::async_connect( socket, endpoints,
                        [this]( std::error_code error, asio::ip::tcp::endpoint endpoint )
                        {
                            if( !error )
                            {
                                ReadHeader();
                            }
                            else
                            {
                                std::cout << "Connect to server error: " << error.message() << "\n";
                                socket.close();
                            }
                        } );
                }
            }

            void Disconnect()
            {
                if( IsConnected() )
                    asio::post( asioContext, [this]() { socket.close(); } );
            }

            bool IsConnected() const
            {
                return socket.is_open();
            }

            void Send( const Message <T>& msg )
            {
                asio::post( asioContext,
                    [this, msg]()
                    {
                        bool isWritingMessages = !outMessages.empty();
                        outMessages.push_back( msg );
                        if( !isWritingMessages )
                        {
                            WriteHeader();
                        }
                    } );
            }

        protected:
            asio::io_context& asioContext;
            asio::ip::tcp::socket socket;
            Owner ownerType = Owner::Server;

            ThreadSafeQueue<Message<T>> outMessages;
            ThreadSafeQueue<OwnedMessage<T>>& inMessages;
            Message<T> incomingMessageBuffer;

            uint32_t id = 0;

        private:

            void ReadHeader()
            {
                asio::async_read( socket, asio::buffer( &(incomingMessageBuffer.header), sizeof( MessageHeader<T> ) ),
                    [this]( std::error_code error, std::size_t length )
                    {
                        if( !error )
                        {
                            if( incomingMessageBuffer.header.size > 0 )
                            {
                                incomingMessageBuffer.body.resize( incomingMessageBuffer.header.size );
                                ReadBody();
                            }
                            else
                            {
                                AddToIncomingMessages();
                            }
                        }
                        else
                        {
                            std::cout << "[" << id << "] ReadHeader error: " << error.message() << "\n";
                            socket.close();
                        }
                    } );
            }

            void ReadBody()
            {
                asio::async_read( socket, asio::buffer( incomingMessageBuffer.body.data(), incomingMessageBuffer.body.size() ), //dynamic_vector_buffer?
                    [this]( std::error_code error, std::size_t length )
                    {
                        if( !error )
                        {
                            AddToIncomingMessages();
                        }
                        else
                        {
                            std::cout << "[" << id << "] ReadBody error: " << error.message() << "\n";
                            socket.close();
                        }
                    } );
            }

            void WriteHeader()
            {
                asio::async_write( socket, asio::buffer( &(outMessages.front().header), sizeof( MessageHeader<T>) ),
                    [this]( std::error_code error, std::size_t length )
                    {
                        if( !error )
                        {
                            if( outMessages.front().body.size() > 0 )
                            {
                                WriteBody();
                            }
                            else
                            {
                                outMessages.pop_front();

                                if( !outMessages.empty() )
                                {
                                    WriteHeader();
                                }
                            }
                        }
                        else
                        {
                            std::cout << "[" << id << "] WriteHeader error: " << error.message() << "\n";
                            socket.close();
                        }
                    } );
            }

            void WriteBody()
            {
                asio::async_write( socket, asio::buffer( outMessages.front().body.data(), outMessages.front().body.size() ),
                    [this]( std::error_code ec, std::size_t length )
                    {
                        if( !ec )
                        {
                            outMessages.pop_front();

                            if( !outMessages.empty() )
                            {
                                WriteHeader();
                            }
                        }
                        else
                        {
                            std::cout << "[" << id << "] WriteBody fail.\n";
                            socket.close();
                        }
                    } );
            }

            void AddToIncomingMessages()
            {
                if( ownerType == Owner::Server )
                    inMessages.push_back( { this->shared_from_this(), incomingMessageBuffer } );
                else
                    inMessages.push_back( { nullptr, incomingMessageBuffer } );

                ReadHeader();
            }
        };
    }
}