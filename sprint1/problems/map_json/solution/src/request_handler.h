#pragma once
#include "http_server.h"
#include "model.h"

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }


    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;
    
    StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
        bool keep_alive,
        boost::beast::string_view content_type = "text/html") {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
       
        const auto text_response = [&req, this](http::status status, std::string_view text) {
        return this->MakeStringResponse(status, text, req.version(), req.keep_alive());
        };
        if (req.method_string() == "GET")
        {
            std::string str1 = "Hello, ";
            std::string targ = static_cast<std::string> (req.target().substr(1, req.target().size() - 1));
            std::string str2 = "";
           send(text_response(http::status::ok, str1 + targ + str2));
           return;
        }
        if (req.method_string() == "HEAD") {
            send(text_response(http::status::ok, ""));
            return;
        }
        auto response = text_response(http::status::method_not_allowed, "Invalid method");
        response.set(http::field::allow, "GET, HEAD");
      send (response);
    }

private:
    model::Game& game_;
};

}  // namespace http_handler

//StringResponse HandleRequest(StringRequest&& req) {
//    const auto text_response = [&req](http::status status, std::string_view text) {
//        return MakeStringResponse(status, text, req.version(), req.keep_alive());
//        };
//    if (req.method_string() == "GET")
//    {
//        std::string str1 = "Hello, ";
//        std::string targ = static_cast<std::string> (req.target().substr(1, req.target().size() - 1));
//        std::string str2 = "";
//        return text_response(http::status::ok, str1 + targ + str2);
//    }
//    if (req.method_string() == "HEAD")
//        return text_response(http::status::ok, "");
//    auto response = text_response(http::status::method_not_allowed, "Invalid method");
//    response.set(http::field::allow, "GET, HEAD");
//    return response;
//    // return MakeStringResponse(http::status::ok, "OK"sv, req.version(), req.keep_alive());
//}