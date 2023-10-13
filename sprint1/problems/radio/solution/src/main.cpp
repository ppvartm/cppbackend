
#include <boost/asio.hpp>
#include "audio.h"

#include <iostream>
#include <string>

using namespace std::literals;

namespace net = boost::asio;
using net::ip::udp;

void Start_Client(uint16_t port)
{
    std::string adr;
    std::cout << "PLease, input server ip: "sv;
    std::cin >> adr;
    try
    {
        boost::system::error_code ec;
        auto ip = net::ip::make_address(adr, ec);


        net::io_context io_context;
        udp::socket socket(io_context, udp::v4());

        auto endpoint = udp::endpoint(ip, port);

        Recorder recorder(ma_format_u8, 1);

        while (true)
        {

            std::cout << "Record message..." << std::endl;


            auto record = recorder.Record(60000, 1.5s);
            std::vector<char> rec_result = record.data;
            std::cout << "Recording done" << std::endl;
            socket.send_to(net::buffer(rec_result), endpoint);


            std::array<char, 1024> recv_buf;
            udp::endpoint sender_endpoint;
            size_t size = socket.receive_from(net::buffer(recv_buf), sender_endpoint);

            std::cout << "Server responded "sv << std::string_view(recv_buf.data(), size) << std::endl;
            std::this_thread::sleep_for(3s);
        }
    }
    catch (std::exception& ec)
    {
        std::cout << ec.what() << "\n";
    }
}

void Start_Server(uint16_t port)
{
    try {
        net::io_context io_context;
        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));
        Player player(ma_format_u8, 1);
        while (true)
        {
            udp::endpoint remote_endpoint;
            std::vector<char> rec_result(60000);
            socket.receive_from(boost::asio::buffer(rec_result), remote_endpoint);

            std::cout << rec_result.size() << "\n";
            std::cout << "Client said \n"sv;
            player.PlayBuffer(rec_result.data(), rec_result.size() / player.GetFrameSize(), 1.5s);

            boost::system::error_code ignored_error;
            socket.send_to(boost::asio::buffer("Record is getted"sv), remote_endpoint, 0, ignored_error);
        }
    }
    catch (std::exception& ex)
    {
        std::cout << ex.what() << "\n";
    }
}



int main(int argc, char** argv) {
    Recorder recorder(ma_format_u8, 1);
    Player player(ma_format_u8, 1);

    const static uint16_t port = std::stoi(std::string(argv[2]));

   
    if (std::string(argv[1]) == "client")
    {
        Start_Client(port);
    }
    if (std::string(argv[1]) == "server")
    {
        Start_Server(port);
    }



    /*while (true) {
        std::string str;

        std::cout << "Press Enter to record message..." << std::endl;
        std::getline(std::cin, str);

        auto rec_result = recorder.Record(130000, 3s);
        std::cout << "Recording done" << std::endl;

        player.PlayBuffer(rec_result.data.data(), rec_result.frames, 3s);
        std::cout << "Playing done" << std::endl;
    }*/

    return 0;
}
