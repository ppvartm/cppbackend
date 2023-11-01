#pragma once
#include "http_server.h"
#include "boost/json.hpp"
#include "model.h"
#include <filesystem>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace fs = std::filesystem;

json::value BadRequest();
json::value MapNotFound();
std::string GetMaps(const model::Game& game);
fs::path GetFilePath(const std::string& file_name);
std::string GetFileType(std::string file_name);
bool IsFileExist(const fs::path& file_path);
bool IsAccessibleFile(fs::path file_path, fs::path base_path);
http::file_body::value_type ReadFile(const fs::path& file_path);
std::string UrlDeCode(const std::string& url_path);

struct MapInfo {
    model::Map::Id id_;
    std::string name_;
    MapInfo(const model::Map& map):id_(map.GetId()), name_(map.GetName()) {
    }
};

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }
     // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;
    // Ответ, тело которого представлено в виде файла
    using FileResponse = http::response<http::file_body>;
    
    StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
        bool keep_alive,
        boost::beast::string_view content_type) {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    FileResponse MakeFileResponse(http::status status, fs::path file_path, unsigned http_version,
        bool keep_alive,
        std::string content_type) {
        FileResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = ReadFile(file_path);
        response.prepare_payload();
        return response;
    }

    void SetFilePath(std::string file_path) {
        path_ = "../../" + file_path;
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
       
        const auto text_response = [&req, this](http::status status, std::string_view text, boost::beast::string_view content_type ) {
        return this->MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type);
            };
        const auto file_response = [&req, this](http::status status, fs::path file_path, std::string content_type) {
            return this->MakeFileResponse(status, file_path, req.version(), req.keep_alive(), content_type);
            };

        
        if ((req.method_string() == "GET") && 
            (static_cast<std::string>(req.target())=="/api/v1/maps"))
        {
            std::string answ = GetMaps(game_);
           send(text_response(http::status::ok, answ, "application/json"));
           return;
        }
        if ((req.method_string() == "GET") && 
            (static_cast<std::string>(req.target()).substr(0, 16) == "/api/v1/maps/map"))
        {
            std::string id = static_cast<std::string>(req.target()).substr(13); //получаем номер карты
            auto map = game_.FindMap(model::Map::Id(id)); //ищем карту
            if (map) {
                std::string answ = json::serialize(json::value_from(*map));
               send(text_response(http::status::ok, answ, "application/json"));
                return;
            }
            else {
                std::string answ = json::serialize(MapNotFound());  //Карты не нашлось
                send(text_response(http::status::not_found, answ, "application/json"));
                return;
            }
        }
        if (((req.method_string() == "GET") || (req.method_string() == "HEAD")) &&((
            (static_cast<std::string>(req.target()).substr(0, 4) != "/api") && 
            (static_cast<std::string>(req.target()) != "/favicon.ico") && 
            (static_cast<std::string>(req.target()) != "/")) ||
            (static_cast<std::string>(req.target()) == "/")
            ))
        {
            std::filesystem::path file_path;
            if (static_cast<std::string>(req.target()) != "/");
            file_path = path_.string() + UrlDeCode(static_cast<std::string>(req.target()));
            if (static_cast<std::string>(req.target()) == "/")
            file_path = path_.string() + "/index.html";
            if (IsAccessibleFile(file_path, path_)) {
                if (!IsFileExist(file_path)) {
                     send(text_response(http::status::not_found, "File not found", "text/plain"));
                     return;
                }
                std::string file_type = GetFileType(file_path.string());
                send(file_response(http::status::ok, file_path, file_type));
                return;
            }
            else {
                send(text_response(http::status::bad_request, "Not access", "text/plain"));
                return;
            }          
        }
       std::string answ = json::serialize(BadRequest());   //Невалидный запрос
       send(text_response(http::status::bad_request, answ, "application/json"));
       return;
    }

private:
    model::Game& game_;
    std::filesystem::path path_;
 };

}  // namespace http_handler
