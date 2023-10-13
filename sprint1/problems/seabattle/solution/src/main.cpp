#ifdef WIN32
#include <sdkddkver.h>
#endif

#include "seabattle.h"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <string_view>

namespace net = boost::asio;
using net::ip::tcp;
using namespace std::literals;

void PrintFieldPair(const SeabattleField& left, const SeabattleField& right) {
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i) {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket& socket) {
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec) {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket& socket, std::string_view data) {
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()), ec);

    return !ec;
}

class SeabattleAgent {
public:
    SeabattleAgent(const SeabattleField& field)
        : my_field_(field) {
    }

    void StartGame(tcp::socket& socket, bool my_initiative) {
        while (!IsGameEnded())
        {
            if (my_initiative)
            {
                PrintFields(); //печать полей
                std::cout << "Your move: "sv;
                std::string move;
                std::cin >> move;  // текущий ход
    
                WriteExact(socket, move);
                auto my_move_result = ReadExact<1>(socket);
                if (my_move_result.value() == "2")
                {
                    std::cout << "U'rs shot is killed! \n";
                    other_field_.MarkKill(ParseMove(move).value().first, ParseMove(move).value().second);
                }
                if (my_move_result.value() == "1")
                {
                    std::cout << "U'rs shot is hitted! \n";
                    other_field_.MarkHit(ParseMove(move).value().first, ParseMove(move).value().second);
                }
                if (my_move_result.value() == "0")
                {
                    std::cout << "U'rs shot is lose! \n";
                    other_field_.MarkHit(ParseMove(move).value().first, ParseMove(move).value().second);
                    my_initiative = false;
                }
            }
            else
            {
                PrintFields();
                std::cout << "Expect your opponent's move: \n "sv;
                auto res_of_opponent_move = ReadExact<2>(socket);
                auto coordinates_on_field = ParseMove(res_of_opponent_move.value());
                int result_of_opponent_move = static_cast<int>(my_field_.Shoot(coordinates_on_field.value().first, coordinates_on_field.value().second));
                if (result_of_opponent_move == 0) my_initiative = true;
                WriteExact(socket, std::to_string(result_of_opponent_move));
            }

        }
        std::cout << "GAME OVER!\n";
        int a;
        std::cin >> a;
    }

private:
    static std::optional<std::pair<int, int>> ParseMove(const std::string_view& sv) {
        if (sv.size() != 2) return std::nullopt;

        int p1 = sv[0] - 'A', p2 = sv[1] - '1';

        if (p1 < 0 || p1 > 8) return std::nullopt;
        if (p2 < 0 || p2 > 8) return std::nullopt;

        return {{p1, p2}};
    }

    static std::string MoveToString(std::pair<int, int> move) {
        char buff[] = {static_cast<char>(move.first) + 'A', static_cast<char>(move.second) + '1'};
        return {buff, 2};
    }

    void PrintFields() const {
        PrintFieldPair(my_field_, other_field_);
    }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    void send_move(tcp::socket& socket, std::string& str)
    {
        socket.write_some(net::buffer(str));
     //   if (ec) { std::cout << "Error sending data\n"sv; }
    }

    std::string read_move(tcp::socket& socket)
    {
        net::streambuf stream_buf;     
        net::read_until(socket, stream_buf, '\n');
        std::string server_data{ std::istreambuf_iterator<char>(&stream_buf), std::istreambuf_iterator<char>() };
        return server_data;
    }
   
    void send_result(tcp::socket& socket, char ch)
    {
        socket.write_some(net::buffer(&ch, 1));
    }

    std::string read_result(tcp::socket& socket)
    {
        net::streambuf stream_buf;
        net::read_until(socket, stream_buf, '\n');
        std::string server_data{ std::istreambuf_iterator<char>(&stream_buf), std::istreambuf_iterator<char>() };
        return server_data;
    }

private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField& field, unsigned short port) {
    SeabattleAgent agent(field);

    net::io_context io_context;  //создаем контекст
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port)); //создаем акцептор, так как к нам будут подключаться

    tcp::socket socket(io_context);  //создаем сокет - это наш интерфейс для работы с клиентом
    acceptor.accept(socket);   //принимаем подключение (фунция ждет, пока клиент не подключится)
    

    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField& field, const std::string& ip_str, unsigned short port) {
    SeabattleAgent agent(field);

    auto endpoint = tcp::endpoint(net::ip::make_address(ip_str), port); //определяем сервер, к которому будем подключаться

    net::io_context io_context;  //создаем контекст
    tcp::socket socket(io_context); //создаем сокет из контекста
    socket.connect(endpoint);  //подключаем сокет к серверу

    agent.StartGame(socket, true);

};

int main(int argc, const char** argv) {
    if (argc != 3 && argc != 4) {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    if (argc == 3) {
        StartServer(fieldL, std::stoi(argv[2]));
    } else if (argc == 4) {
        StartClient(fieldL, argv[2], std::stoi(argv[3]));
    }
}
