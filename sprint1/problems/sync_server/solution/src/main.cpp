#ifdef WIN32
#include <sdkddkver.h>
#endif
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <thread>
#include <optional>

namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

//Функция для чтения запроса от клиента
std::optional<StringRequest> ReadRequest(tcp::socket& socket, beast::flat_buffer& buffer) {
    beast::error_code ec;
    StringRequest req;
    http::read(socket, buffer, req, ec);

    if (ec == http::error::end_of_stream) return std::nullopt;
    if (ec) throw std::runtime_error("Failed to read request:"s.append(ec.message()));

    return req;
}

//Печать содержимого запроса от клиента
void DumpRequest(const StringRequest& req)
{
    std::cout << req.method_string() << "  " << req.target() << "\n"sv;
    for (const auto& header : req)
        std::cout << "  "sv << header.name_string() << ": "sv << header.value() << "\n"sv;
}

StringResponse HandleRequest(StringRequest&& req) //понять почему по правой ссылке
{

}


template<typename RequestHandler>
void HandleConnection(tcp::socket& socket, RequestHandler&& HandleRequest)
{
    try {
        beast::flat_buffer buffer;

        while (auto request = ReadRequest(socket, buffer)) {
            DumpRequest(request.value());
            StringResponse = HandleRequest(std::move(request).value());
        }
    }
    catch (std::exception& ex) {

    }

}
int main() {
    // Выведите строчку "Server has started...", когда сервер будет готов принимать подключения
    net::io_context io_context;
    constexpr unsigned short port = 8080;
    auto address = net::ip::make_address("0.0.0.0");

    tcp::acceptor acceptor(io_context, { address, port });
    std::cout << "Server has started..."sv;
    for (;;)
    {
        tcp::socket socket(io_context);
        acceptor.accept(socket);
        std::thread t([](tcp::socket socket) { HandleConnection(socket); }, std::move(socket));
        t.detach();
    }


}
