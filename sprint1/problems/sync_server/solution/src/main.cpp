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

struct ContentType {
    ContentType() = delete;
    constexpr static beast::string_view TEXT_HTML = "text/html";
};

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

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
    bool keep_alive,
    std::string_view content_type = ContentType::TEXT_HTML) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}



StringResponse HandleRequest(StringRequest&& req) //понять почему по правой ссылке
{
    
    const auto text_response = [&req](http::status status, std::string_view text) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive());
        };
    if (req.method_string() == "GET")
    {
        std::string str1 = "<strong>Hello, ";
        std::string targ = static_cast<std::string> (req.target().substr(1, req.target().size() - 1));
        std::string str2 = "</strong>";
        return text_response(http::status::ok, str1 + targ + str2);
    }
    if (req.method_string() == "HEAD")
        return text_response(http::status::ok, "");
    auto response = text_response(http::status::method_not_allowed, "Invalid method");
    response.set(http::field::allow, "GET, HEAD");
    return response;
}


template<typename RequestHandler>
void HandleConnection(tcp::socket& socket, RequestHandler&& HandleRequest)
{
    try {
        beast::flat_buffer buffer;

        while (auto request = ReadRequest(socket, buffer)) {
            DumpRequest(request.value());
            StringResponse response = HandleRequest(std::move(request).value());
            http::write(socket, response);
            if (response.need_eof()) break;
        }
    }
    catch (std::exception& ex) {
        std::cerr << ex.what() << "\n"sv;
    }
    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_send, ec);
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
        std::thread t([](tcp::socket socket) { HandleConnection(socket, HandleRequest); }, std::move(socket));
        t.detach();
    }


}
