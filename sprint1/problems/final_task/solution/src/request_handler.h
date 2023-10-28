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
        boost::beast::string_view content_type = "application/json") {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;
    //Вероятно, есть способ записать ответ средствами boost/json, но я не очень понял, как это сделать в случае, если информация хранится
    // в полях структуры 
    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
       
        const auto text_response = [&req, this](http::status status, std::string_view text) {
        return this->MakeStringResponse(status, text, req.version(), req.keep_alive());
        };
        if ((req.method_string() == "GET")&& (static_cast<std::string>(req.target())=="/api/v1/maps"))
        {
            std::string answ="[";
            for (auto& it: game_.GetMaps())
            {
                answ += "{\"id\": \"" + *it.GetId() + "\", \"name\": \"" + it.GetName() + "\"},";
            }
            answ.erase(answ.size()-1, answ.size());
            answ += "]";
           send(text_response(http::status::ok, answ));
           return;
        }
        if ((req.method_string() == "GET") && (static_cast<std::string>(req.target()).substr(0, 16) == "/api/v1/maps/map"))
        {
            std::string id = static_cast<std::string>(req.target()).substr(13); //получаем номер карты
            auto map = game_.FindMap(model::Map::Id(id)); //ищем карту
            if (map) {
                std::string answ = "{\n\"id\": \"" + *(map->GetId()) + "\",\n" +
                    "\"name\": \"" + map->GetName() + "\",\n" +
                    "\"roads\": [\n";
                for (auto& it : map->GetRoads()) {
                    answ += "{ \"x0\": " + std::to_string(it.GetStart().x) + ", \"y0\": " + std::to_string(it.GetStart().y) + ",";
                    if (it.IsHorizontal()) answ += "\"x1\": " + std::to_string(it.GetEnd().x) + " },\n";
                    if (it.IsVertical()) answ += "\"y1\": " + std::to_string(it.GetEnd().y) + " },\n";
                }
                answ.erase(answ.size() - 2, answ.size());
                answ += "\n],\n \"buildings\": [\n";
                for (auto& it : map->GetBuildings()) {
                    answ += "{ \"x\": " + std::to_string(it.GetBounds().position.x) + ", \"y\": " + std::to_string(it.GetBounds().position.y) + ", \"w\": " +
                        std::to_string(it.GetBounds().size.width) + ", \"h\": " + std::to_string(it.GetBounds().size.height) + " },\n";
                }
                answ.erase(answ.size() - 2, answ.size());
                answ += "\n],\n \"offices\": [\n";
                for (auto& it : map->GetOffices()) {
                    answ += "{ \"id\": \"" + *it.GetId() + "\", \"x\": " + std::to_string(it.GetPosition().x) + ", \"y\": " + std::to_string(it.GetPosition().y) +
                        ", \"offsetX\": " + std::to_string(it.GetOffset().dx) + ", \"offsetY\": " + std::to_string(it.GetOffset().dy) + " },\n";
                }
                answ.erase(answ.size() - 2, answ.size());
                answ += "\n]\n}";
                send(text_response(http::status::ok, answ));
                return;
            }
            else {
                std::string answ = "{\n  \"code\": \"mapNotFound\",\n  \"message\": \"Map not found\"\n}"; //карта не нашлась
                send(text_response(http::status::not_found, answ));
                return;
            }
        }
       std::string answ = "{\n  \"code\": \"badRequest\",\n  \"message\": \"Bad request\"\n}";
       send(text_response(http::status::bad_request, answ));
       return;
    }

private:
    model::Game& game_;
};

}  // namespace http_handler
