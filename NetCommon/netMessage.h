#ifndef NETMESSAGE_H
#define NETMESSAGE_H

#include "netCommon.h"

namespace Vel
{
	namespace Net
	{
		template <typename T>
		struct MessageHeader
		{
			T id{};
			uint32_t size = 0;
		};

		template <typename T>
		struct Message
		{
			MessageHeader<T> header;
			std::vector<uint8_t> body;

			size_t size() const
			{
				return sizeof( MessageHeader<T> ) + body.size();
			}

			friend std::ostream& operator << ( std::ostream& os, const Message<T> & msg )
			{
				os << "ID: " << int( msg.header.id ) << " Size: " << msg.header.size;
				return os;
			}

			template <typename DataType>
			friend Message<T>& operator << ( Message<T>& msg, const DataType& data )
			{
				static_assert( std::is_standard_layout<DataType>::value, "Data is not serializable" );

				size_t currentSize = msg.body.size();
				msg.body.resize( currentSize + sizeof( DataType ) );

				std::memcpy( msg.body.data() + currentSize, &data, sizeof( DataType ) );

				msg.header.size = msg.size();

				return msg;
			}

			template <typename DataType>
			friend Message<T>& operator >> ( Message<T>& msg, DataType& data )
			{
				static_assert( std::is_standard_layout<DataType>::value, "Data is not serializable" );

				size_t dataPosition = msg.body.size() - sizeof( DataType );

				std::memcpy( &data, msg.body.data() + dataPosition, sizeof( DataType ) );

				msg.body.resize( dataPosition );

				msg.header.size = dataPosition;

                return msg;
			}
		};

        template <typename T>
        class Connection;

        template <typename T>
        struct OwnedMessage
        {
            std::shared_ptr<Connection<T>> remote = nullptr;
            Message<T> msg;

            friend std::ostream& operator << ( std::ostream& os, const OwnedMessage<T>& msg )
            {
                os << msg.msg;
                return os;
            }
        };
	}
}

#endif