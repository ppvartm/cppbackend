#pragma once
#include "http_server.h"
#include "boost/json.hpp"
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
json::value InvalidContentType(); 
json::value ErrorParseAction();

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
    explicit RequestHandler(model::Game& game)
        : game_{ game } {
    }
   
    StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version, bool keep_alive, boost::beast::string_view content_type);
    FileResponse GETMakeFileResponse(http::status status, fs::path file_path, unsigned http_version, bool keep_alive, std::string content_type);
    FileResponse HEADMakeFileResponse(http::status status, fs::path file_path, unsigned http_version, bool keep_alive, std::string content_type);

    void SetFilePath(fs::path file_path);

    bool CheckAuthorization(const app::Token& tocken);



    template <typename Body, typename Allocator, typename Send, typename Fn >
    std::optional<StringResponse> API_PerfomActionWithToken(const http::request<Body, http::basic_fields<Allocator>>& req, Send& send, Fn&& func) {
        const auto json_text_response = [&req, this](json::value&& jv, http::status status) {
            std::string answ = json::serialize(jv);
            StringResponse response = MakeStringResponse(status, answ, req.version(), req.keep_alive(), "application/json");
            response.set(http::field::cache_control, "no-cache");
            return response;
            };
        auto auth = req.base()["Authorization"].to_string();
        try {
            if (auth.substr(7).size() != 32)
                throw std::exception();
        }
        catch (...) {
            auto response = json_text_response(AuthorizationMissing(), http::status::unauthorized);
            send(response);
            return response;
        }
        auto token_from_req = auth.substr(7);
        if (CheckAuthorization(token_from_req)) {
            auto answer = func(token_from_req);
            if (answer == ErrorParseAction()){
                auto response = json_text_response(std::move(answer), http::status::bad_request);
                send(response);
                return response;
            }
           auto response = json_text_response(std::move(answer), http::status::ok);
           send(response);
           return response;
        }
        else {
            auto response = json_text_response(PlayerNotFound(), http::status::unauthorized);
            send(response);
            return response;
        }
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
            std::string userName;
            std::string mapId;
            try {
                 userName = jv.as_object().at("userName").as_string();
                 mapId = jv.as_object().at("mapId").as_string();
            }
            catch (...) {
                auto response = json_text_response(EmptyNickname(), http::status::bad_request);
                send(response);
                return response;
            }
            //ошибка пустого имени
            if (userName == "") {
                auto response = json_text_response(EmptyNickname(), http::status::bad_request);
                send(response);
                return response;
            }

            //ошибка отсутствия карты
            if (!game_.FindMap(model::Map::Id(static_cast<std::string>(mapId)))) {
                auto response = json_text_response(MapNotFound(), http::status::not_found);
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
           
            return API_PerfomActionWithToken(req, send, [this](const app::Token& token) {
                m.lock();
                auto gs = this->tokens_.FindPlayerByToken(token)->GetGameSession();
                json::value name = {
                     {"name", gs->GetDogs().begin()->second->GetName()}
                };
                json::value answer = { {std::to_string(gs->GetDogs().begin()->second->GetId()), name} };
                for (auto p = (gs->GetDogs().begin()); p != gs->GetDogs().end(); ++p) {
                    name = {
                     {"name", p->second->GetName()}
                    };
                    answer.get_object().emplace(std::to_string(p->second->GetId()), name);
                }
                m.unlock();
                return answer; });

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
    std::optional<StringResponse> API_GameState_RequestHand(const http::request<Body, http::basic_fields<Allocator>>& req, Send& send) {
        const auto json_text_response = [&req, this](json::value&& jv, http::status status) {
            std::string answ = json::serialize(jv);
            StringResponse response = MakeStringResponse(status, answ, req.version(), req.keep_alive(), "application/json");
            response.set(http::field::cache_control, "no-cache");
            return response;
            };
        if (((req.method_string() == "GET") || (req.method_string() == "HEAD")) &&
            (static_cast<std::string>(req.target()) == "/api/v1/game/state")) {

           return API_PerfomActionWithToken(req, send, [this](const app::Token& token) {
                auto gs = this->tokens_.FindPlayerByToken(token)->GetGameSession();
                json::value information_about_dog = {
                    {"pos", std::vector<double>({gs->GetDogs().begin()->second->GetPosition().x, gs->GetDogs().begin()->second->GetPosition().y})},
                    {"speed",std::vector<double>({gs->GetDogs().begin()->second->GetSpeed().s_x, gs->GetDogs().begin()->second->GetSpeed().s_y}) },
                    {"dir", gs->GetDogs().begin()->second->GetDirectionToString()}
                };
                json::value players = {
                    {std::to_string(gs->GetDogs().begin()->second->GetId()), information_about_dog}
                };
                for (auto p = (gs->GetDogs().begin()); p != gs->GetDogs().end(); ++p) {
                    information_about_dog = {
                       {"pos", {p->second->GetPosition().x, p->second->GetPosition().y}},
                       {"speed",std::vector<double>({p->second->GetSpeed().s_x, p->second->GetSpeed().s_y}) },
                       {"dir", p->second->GetDirectionToString()}
                    };
                    players.get_object().emplace(std::to_string(p->second->GetId()), information_about_dog);
                }
                json::value answer = {
                    { "players", players }
                };
                return answer;
           });           
        }
         else if (static_cast<std::string>(req.target()) == "/api/v1/game/state") {
            auto response = json_text_response(InvalidMethod(), http::status::method_not_allowed);
            response.set(http::field::allow, "GET, HEAD");
            send(response);
            return response;           
         }
         return std::nullopt;
    }
    template <typename Body, typename Allocator, typename Send>
    std::optional<StringResponse> API_MovePlayer_RequestHand(const http::request<Body, http::basic_fields<Allocator>>& req, Send& send) {
        const auto json_text_response = [&req, this](json::value&& jv, http::status status) {
            std::string answ = json::serialize(jv);
            StringResponse response = MakeStringResponse(status, answ, req.version(), req.keep_alive(), "application/json");
            response.set(http::field::cache_control, "no-cache");
            return response;
            };

        if ((req.method_string() == "POST") &&
            (static_cast<std::string>(req.target()) == "/api/v1/game/player/action")) {
            std::cout << "IM here\n";
            try {
                if (req.base()["Content-Type"].to_string() != "application/json")
                    throw std::exception();
            }
            catch (...) {
                auto response = json_text_response(InvalidContentType(), http::status::bad_request);
                send(response);
                return response;
            }

            return API_PerfomActionWithToken(req, send, [this, &req](const app::Token& token) {
                auto player = this->tokens_.FindPlayerByToken(token);
                double dog_speed = this->tokens_.FindPlayerByToken(token)->GetGameSession()->GetDogSpeed();

                json::error_code ec;
                json::value jv = json::parse(req.body(), ec);
                if (ec || !jv.as_object().contains("move"))
                    return ErrorParseAction();
                
                std::string dir = static_cast<std::string>(jv.as_object().at("move").as_string());
                std::lock_guard lg(m);
                if (dir == "L")
                    player->SetDogSpeed( -dog_speed,0. );
                else if (dir == "R")
                    player->SetDogSpeed( dog_speed,0. );
                else if (dir == "U")
                    player->SetDogSpeed( 0., -dog_speed );
                else if (dir == "D")
                    player->SetDogSpeed( 0., dog_speed );
                else if (dir == "")
                    player->SetDogSpeed( 0.,0. );
                else {
                    return ErrorParseAction();
                }
                json::value answer = json::object();
                return answer;
            });

        }
        else if (static_cast<std::string>(req.target()) == "/api/v1/game/player/action") {
            auto response = json_text_response(NotPostRequest(), http::status::method_not_allowed);
            response.set(http::field::allow, "POST");
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

        if (auto api_state_game = API_GameState_RequestHand(req, send))
            return *api_state_game;

        if (auto api_move_players = API_MovePlayer_RequestHand(req, send))
            return *api_move_players;


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