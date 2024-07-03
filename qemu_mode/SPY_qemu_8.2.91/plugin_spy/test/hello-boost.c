#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

int main()
{
    try
    {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8888));

        while (true)
        {
            tcp::socket socket(io_context);
            acceptor.accept(socket);

            std::string message = "Hello, client!\n";
            boost::asio::write(socket, boost::asio::buffer(message));
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}