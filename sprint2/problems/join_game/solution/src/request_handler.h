#pragma once
#include "http_server.h"
#include "boost/json.hpp"
#include <boost/asio/io_context.hpp>
#include "model.h"
#include <filesystem>
#include "log.h"
#include "ctime"
#include <variant>
#include "app.h"
#include <mutex>

namespace http_handler {

  

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace fs = std::filesystem;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;

json::value BadRequest();
json::value MapNotFound();
json::value EmptyNickname();
json::value JsonParseError();
json::value NotPostRequest();
json::value AuthorizationMissing();
json::value PlayerNotFound();
json::value InvalidMethod();

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
    using Strand = boost::asio::strand<boost::asio::io_context::executor_type>;
    explicit RequestHandler(model::Game& game, Strand& api_strand)
        : game_{ game }, strand_(api_strand) {
    }
   
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

    FileResponse GETMakeFileResponse(http::status status, fs::path file_path, unsigned http_version,
        bool keep_alive,
        std::string content_type) {
        FileResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = ReadFile(file_path);
        response.prepare_payload();
        return response;
    }
    FileResponse HEADMakeFileResponse(http::status status, fs::path file_path, unsigned http_version,
        bool keep_alive,
        std::string content_type) {
        FileResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.content_length(0);
        return response;
    }

    void SetFilePath(fs::path file_path) {
        path_ = file_path;
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    std::optional<StringResponse> GetAPI_info_RequestHand(const http::request<Body, http::basic_fields<Allocator>>& req, Send& send) {
        const auto text_response = [&req, this](http::status status, std::string_view text, boost::beast::string_view content_type) {
            return this->MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type);
            };
        if ((req.method_string() == "GET") &&
            (static_cast<std::string>(req.target()) == "/api/v1/maps"))
        {
            std::string answ = GetMaps(game_);
            auto response = text_response(http::status::ok, answ, "application/json");
            send(response);
            return response;
        }
        if ((req.method_string() == "GET") &&
            (static_cast<std::string>(req.target()).substr(0, 16) == "/api/v1/maps/map"))
        {
            std::string id = static_cast<std::string>(req.target()).substr(13); //получаем номер карты
            auto map = game_.FindMap(model::Map::Id(id)); //ищем карту
            if (map) {
                std::string answ = json::serialize(json::value_from(*map));
                auto response = text_response(http::status::ok, answ, "application/json");
                send(response);
                return response;
            }
            else {
                std::string answ = json::serialize(MapNotFound());  //Карты не нашлось
                auto response = text_response(http::status::not_found, answ, "application/json");
                send(response);
                return response;
            }
        }
        return std::nullopt;
    }
    template <typename Body, typename Allocator, typename Send>
    std::optional<std::variant<StringResponse,FileResponse>> GetStaticFile_RequestHand(const http::request<Body, http::basic_fields<Allocator>>& req, Send& send) {
        const auto text_response = [&req, this](http::status status, std::string_view text, boost::beast::string_view content_type) {
            return this->MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type);
            };
        const auto get_file_response = [&req, this](http::status status, fs::path file_path, std::string content_type) {
            return this->GETMakeFileResponse(status, file_path, req.version(), req.keep_alive(), content_type);
            };
        const auto head_file_response = [&req, this](http::status status, fs::path file_path, std::string content_type) {
            return this->HEADMakeFileResponse(status, file_path, req.version(), req.keep_alive(), content_type);
            };
        if (((req.method_string() == "GET") || (req.method_string() == "HEAD")) && ((
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
            file_path = fs::weakly_canonical(file_path);
            if (IsAccessibleFile(file_path, path_)) {
                if (!IsFileExist(file_path)) {
                    auto response = text_response(http::status::not_found, "File not found", "text/plain");
                    send(response);
                    return response;
                }
                std::string file_type = GetFileType(file_path.string());
                if (req.method_string() == "GET") {
                    auto response = get_file_response(http::status::ok, file_path, file_type);
                    send(response);
                    return response;
                }
                if (req.method_string() == "HEAD") {
                    auto response  = head_file_response(http::status::ok, file_path, file_type);
                    send(response);
                    return response;
                }
            }
            else {
                auto response = text_response(http::status::bad_request, "Not access", "text/plain");
                send(response);
                return response;
            }
        }

        return std::nullopt;
    }
    template <typename Body, typename Allocator, typename Send>
    std::optional<StringResponse> API_AuthGame_RequestHand(const http::request<Body, http::basic_fields<Allocator>>& req, Send& send) {
        const auto json_text_response = [&req, this](json::value&& jv, http::status status) {
            std::string answ = json::serialize(jv);
            StringResponse response = MakeStringResponse(status, answ, req.version(), req.keep_alive(), "application/json");
            response.set(http::field::cache_control, "no-cache");
            return response;
            };

        if ((req.method_string() == "POST") &&
            (static_cast<std::string>(req.target()) == "/api/v1/game/join")) {
            json::error_code ec;
            json::value jv = json::parse(req.body(), ec);
            //ошибка парсинга
            if (ec) {
                auto response = json_text_response(JsonParseError(),http::status::bad_request);
                send(response);
                return response;
            }
            
            auto userName = jv.as_object().at("userName").as_string();
            auto mapId = jv.as_object().at("mapId").as_string();

            //ошибка отсутствия карты
            if (!game_.FindMap(model::Map::Id(static_cast<std::string>(mapId)))) {
                auto response = json_text_response(MapNotFound(), http::status::not_found);
                send(response);
                return response;
           }
            //ошибка пустого имени
            if (userName == "") {
                auto response = json_text_response(EmptyNickname(), http::status::bad_request);
                send(response);
                return response;
            }
            //установка игрока
            m.lock();
            std::shared_ptr<model::GameSession> gs = game_.FindGameSession(model::Map::Id(static_cast<std::string>(mapId)));
            auto players_token = tokens_.AddPlayer(players_.Add(std::make_shared<model::Dog>(static_cast<std::string>(userName)),gs));
            m.unlock();
            jv = {
                {"authToken", players_token},
                {"playerId", (*tokens_.FindPlayerByToken(players_token)).GetDogId()}
            };
            auto response = json_text_response(std::move(jv), http::status::ok);
            send(response);
            return response;
        }
        else if (static_cast<std::string>(req.target()) == "/api/v1/game/join") {
            auto response = json_text_response(NotPostRequest(), http::status::method_not_allowed);
            response.set(http::field::allow, "POST");
            send(response);
            return response;
        }
        return std::nullopt;
    }
    template <typename Body, typename Allocator, typename Send>
    std::optional<StringResponse> API_PlayersList_RequestHand(const http::request<Body, http::basic_fields<Allocator>>& req, Send& send) {
        const auto json_text_response = [&req, this](json::value&& jv, http::status status) {
            std::string answ = json::serialize(jv);
            StringResponse response = MakeStringResponse(status, answ, req.version(), req.keep_alive(), "application/json");
            response.set(http::field::cache_control, "no-cache");
            return response;
            };
        if (((req.method_string() == "GET") || (req.method_string() == "HEAD"))&&
            (static_cast<std::string>(req.target()) == "/api/v1/game/players")) {

            auto auth = req.base()["Authorization"].to_string();
            if ((auth == "") || auth.substr(7).size() != 32) {
                auto response = json_text_response(AuthorizationMissing(), http::status::unauthorized);
                send(response);
                return response;
           }

            auto token_from_req = req.base()["Authorization"].to_string().substr(7);
            //находим игрока, если у него есть доступ к игре (есть токен)
            if (auto player = tokens_.FindPlayerByToken(token_from_req)) {
                auto gs = player->GetGameSession();
                m.lock();
                json::value name = {
                     {"name", gs->GetDogs().begin()->second->GetName()}
                };
                json::value answer = { {std::to_string(gs->GetDogs().begin()->second->GetId()), name} };
                 for(auto p = (gs->GetDogs().begin()); p != gs->GetDogs().end(); ++p) {
                    name = {
                     {"name", p->second->GetName()}
                    };
                    answer.get_object().emplace(std::to_string(p->second->GetId()), name);
                }
                auto response = json_text_response(std::move(answer), http::status::ok);
                send(response);
                m.unlock();
                return response;
            }
            else {
                auto response = json_text_response(PlayerNotFound(), http::status::unauthorized);
                send(response);
                return response;
            } 
        }
        else if(static_cast<std::string>(req.target()) == "/api/v1/game/players"){
            auto response = json_text_response(InvalidMethod(), http::status::method_not_allowed);
            response.set(http::field::allow, "GET, HEAD");
            send(response);
            return response;
        }
        return std::nullopt;
    }

    template <typename Body, typename Allocator, typename Send>
    std::variant<StringResponse, FileResponse> operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {

        const auto text_response = [&req, this](http::status status, std::string_view text, boost::beast::string_view content_type) {
            return this->MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type);
            };
        const auto get_file_response = [&req, this](http::status status, fs::path file_path, std::string content_type) {
            return this->GETMakeFileResponse(status, file_path, req.version(), req.keep_alive(), content_type);
            };
        const auto head_file_response = [&req, this](http::status status, fs::path file_path, std::string content_type) {
            return this->HEADMakeFileResponse(status, file_path, req.version(), req.keep_alive(), content_type);
            }; 

     //'{"userName":  "Scooby Doo", "mapId": "map1"}

        if(auto api_res_info = GetAPI_info_RequestHand(req, send))
           return *api_res_info;

        if (std::optional<std::variant<StringResponse, FileResponse>> static_res = GetStaticFile_RequestHand(req, send)) {
            if (auto file_static_res = std::get_if<FileResponse>(&(*static_res)))
                return std::move(*file_static_res);
            if (auto text_static_res = std::get_if<StringResponse>(&(*static_res)))
                return std::move(*text_static_res);
        }

        if (auto api_auth_game = API_AuthGame_RequestHand(req, send))
            return *api_auth_game;

        if (auto api_players_list_game = API_PlayersList_RequestHand(req, send))
            return *api_players_list_game;


       std::string answ = json::serialize(BadRequest());   //Невалидный запрос
       auto response = text_response(http::status::bad_request, answ, "application/json");
       send(response);
       return response;
    }

private:
    model::Game& game_;
    app::Players players_;
    app::PlayerTokens tokens_;
    std::filesystem::path path_;
    std::mutex m;
    Strand& strand_;
 };

 template <typename RequestHandlerType>
 class LoggingRequestHandler {
 private:

     template <typename Body, typename Allocator>
     void LogRequest(const http::request<Body, http::basic_fields<Allocator>>& req) {
         json::value jv = {
             {"URI", static_cast<std::string>(req.target())},
             {"method", req.method_string()}
         };
         BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, jv) << "request received";
     }

     void LogResponse(const std::variant<StringResponse, FileResponse>& res, int time_of_making_response) {
         json::value jv;
         if(auto p = std::get_if<StringResponse>(&res))
             jv = {
                  {"response_time", time_of_making_response},
                  {"code", p->result_int()},
                  {"content_type", p->base()["Content-Type"].to_string()}
                  };
         else {
             auto pp = std::get_if<FileResponse>(&res);
             jv = {
                  {"response_time", time_of_making_response},
                  {"code", pp->result_int()}, 
                  {"content_type", pp->base()["Content-Type"].to_string()}
             };
         }
         BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, jv) << "response sent";
     }
 public:
     LoggingRequestHandler(RequestHandlerType& rec_handl) :request_handler_(rec_handl) {};
     LoggingRequestHandler(const LoggingRequestHandler&) = delete;
     LoggingRequestHandler& operator=(const LoggingRequestHandler&) = delete;

     template <typename Body, typename Allocator, typename Send>
     void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
         if (static_cast<std::string>(req.target()) != "/favicon.ico"){
             LogRequest(req);
             auto t1 = clock();
             std::variant<StringResponse, FileResponse> resp = request_handler_(std::move(req), std::forward<Send>(send));
             auto t2 = clock();
             LogResponse(resp, int(t2 - t1));
         }
     }

 private:

     RequestHandlerType& request_handler_;
 };


}  // namespace http_handler
