#include "../NetCommon/netCommon.h"

using namespace std::placeholders;

template <std::size_t Capacity>
class CircularBuffer
{
public:
    using const_buffers_type = std::vector<asio::const_buffer>;
    using mutable_buffers_type = std::vector<asio::mutable_buffer>;

    constexpr std::size_t max_size() const
    {
        return Capacity;
    }

    constexpr std::size_t capacity() const //TODO change to distance between tail to head?
    {
        return max_size();
    }

    std::size_t size() const
    {
        return tail - head;
    }

    void commit( std::size_t length )
    {
        tail += length; //Overflow handled in MakeSequence
    }

    void consume( std::size_t length )
    {
        head += length; //Overflow handled in MakeSequence
    }

    auto prepare( std::size_t length )
    {
        if( size() + length > max_size() )
        {
            throw std::length_error( "CircularBuffer overflow" );
        }
        
        return MakeSequence<mutable_buffers_type>( buffer, tail, tail + length );
    }

    auto data() const
    {
        return MakeSequence<const_buffers_type>( buffer, head, tail );
    }

private:
    template<typename Sequence, typename Buffer>
    static Sequence MakeSequence( Buffer& buffer, std::size_t begin, std::size_t end )
    {
        std::size_t size = end - begin;
        begin %= Capacity;
        end %= Capacity;

        if( begin <= end )
        {
            return
            {
                typename Sequence::value_type( &buffer[begin], size )
            };
        }
        else
        {
            std::size_t ending = Capacity - begin;

            return
            {
                typename Sequence::value_type( &buffer[begin], ending ),
                typename Sequence::value_type( &buffer[0], size - ending )
            };
        }
    }

    std::array<char, Capacity> buffer;
    std::size_t head = 0;
    std::size_t tail = 0;
};

template <std::size_t Capacity>
class CircularBufferView
{
public:

    using buffer_type = CircularBuffer<Capacity>;
    using const_buffers_type = typename buffer_type::const_buffers_type;
    using mutable_buffers_type = typename buffer_type::mutable_buffers_type;

    CircularBufferView( buffer_type& buffer )
        : buffer( buffer )
    {}

    auto prepare( std::size_t n )
    {
        return buffer.prepare( n );
    }

    void commit( std::size_t n )
    {
        buffer.commit( n );
    }

    void consume( std::size_t n )
    {
        buffer.consume( n );
    }

    auto data() const
    {
        return buffer.data();
    }

    auto size() const
    {
        return buffer.size();
    }

    constexpr auto max_size() const
    {
        return buffer.max_size();
    }

    constexpr auto capacity() const
    {
        return buffer.capacity();
    }

private:

    buffer_type& buffer;
};

template <std::size_t Capacity>
CircularBufferView<Capacity> MakeCircularBufferView( CircularBuffer<Capacity>& buffer )
{
    return CircularBufferView( buffer );
}

class EchoSession : public std::enable_shared_from_this<EchoSession>
{
public:
    EchoSession( asio::ip::tcp::socket&& socket ) : socket( std::move( socket ) )//, incoming( 65536 )
    {}

    void Start()
    {
        Read();
    }

private:
    void Read()
    {
        asio::async_read( socket, MakeCircularBufferView( buffer ), asio::transfer_at_least( 1 ), std::bind( &EchoSession::OnRead, shared_from_this(), _1, _2 ) );
    }

    void OnRead( std::error_code error, std::size_t bytesTransferred )
    {
        if( !error && bytesTransferred )
        {
            if( !isWriting )
            {
                Write();
            }
            Read();
        }
        else
        {
            std::cout << error.message() << std::endl;
            Close();
        }
    }

    void Write()
    {
        isWriting = true;

        /*auto writeBuffer = outgoing.prepare(incoming.size());
        asio::buffer_copy( writeBuffer, incoming.data() );
        outgoing.commit( writeBuffer.size() );

        incoming.consume( incoming.size() );*/

        asio::async_write( socket, MakeCircularBufferView( buffer ), std::bind( &EchoSession::OnWrite, shared_from_this(), _1, _2 ) );
    }

    void OnWrite( std::error_code error, std::size_t bytesTransferred )
    {
        isWriting = false;

        if( error )
        {
            std::cout << error.message() << std::endl;
            Close();
        }
    }

    void Close()
    {
        std::error_code error;
        socket.close( error );
    }

    asio::ip::tcp::socket socket;
    bool isWriting = false;
    CircularBuffer<65536> buffer;
    //asio::streambuf incoming;
    //asio::streambuf outgoing;
    std::string echoMsg;
};

class EchoServer
{
public:
    EchoServer( asio::io_context& io_context, std::uint16_t port )
        : ioContext( io_context )
        , acceptor( ioContext, asio::ip::tcp::endpoint( asio::ip::tcp::v4(), port ) )
    {}

    void AsyncAccept()
    {
        socket.emplace( ioContext );

        acceptor.async_accept( *socket, [&]( std::error_code error )
            {
                if( !error )
                {
                    std::make_shared<EchoSession>( std::move( *socket ) )->Start();

                    AsyncAccept();
                }
                else
                {
                    std::cout << "Accept error: " << error.message() << "\n";
                }
            } );
    }

private:
    asio::io_context& ioContext;
    asio::ip::tcp::acceptor acceptor;
    std::optional<asio::ip::tcp::socket> socket;
};



class HTTPTest
{
public:
    HTTPTest( asio::io_context& context, const std::string& hostname ) : tcpResolver( context ), socket( context )
    {
        request = "GET / HTTP/1.1\n"
            "Host: " + hostname + "\n"
            "Connection: close\n\n";

        tcpResolver.async_resolve( hostname, "http", std::bind( &HTTPTest::OnResolve, this, std::placeholders::_1, std::placeholders::_2 ) );

    }

private:
    void OnResolve( std::error_code error, asio::ip::tcp::resolver::results_type results )
    {
        std::cout << "Resolve: " << error.message() << std::endl;
        asio::async_connect( socket, results, std::bind( &HTTPTest::OnConnect, this, std::placeholders::_1, std::placeholders::_2 ) );
    }

    void OnConnect( std::error_code error, const asio::ip::tcp::endpoint& endpoint )
    {
        std::cout << "Connect: " << error.message() << std::endl;
        asio::async_write( socket, asio::buffer( request ), std::bind( &HTTPTest::OnWrite, this, std::placeholders::_1, std::placeholders::_2 ) );
    }

    void OnWrite( std::error_code error, size_t bytesTransferred )
    {
        std::cout << "Write: " << error.message() << " Bytes transferred: " << bytesTransferred << std::endl;
        asio::async_read( socket, responseBuffer, std::bind( &HTTPTest::OnRead, this, std::placeholders::_1, std::placeholders::_2 ) );
    }

    void OnRead( std::error_code error, size_t bytesTransferred )
    {
        std::cout << "Read: " << error.message() << ", bytes transferred: " << bytesTransferred << "\n\n" << std::istream( &responseBuffer ).rdbuf() << std::endl;

    }

    asio::ip::tcp::resolver tcpResolver;
    asio::ip::tcp::socket socket;
    asio::streambuf responseBuffer;
    std::string request;
};

int main()
{
    asio::io_context context;

    EchoServer server( context, 13666 );
    server.AsyncAccept();

    context.run();
}
