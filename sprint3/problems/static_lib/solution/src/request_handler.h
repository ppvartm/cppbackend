#pragma once
#include <filesystem>
#include <ctime>
#include <variant>

#include <boost/asio/strand.hpp>
#include "boost/json.hpp"

#include "model.h"
#include "http_server.h"
#include "log.h"
#include "app.h"
#include "extra_data.h"


namespace http_handler {

    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;
    namespace fs = std::filesystem;

    using StringRequest = http::request<http::string_body>;
    using StringResponse = http::response<http::string_body>;
    using FileResponse = http::response<http::file_body>;

    //Создание ответов в json формате
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
    json::value ErrorParseTick();

    std::string GetMaps(const model::Game& game);


    
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
        explicit RequestHandler(model::Game& game, extra_data::Json_data& lost_objects_json_data, Strand& strand)
            : game_{ game },lost_objects_json_data_(lost_objects_json_data), strand_{strand} {
        }
        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version, bool keep_alive, boost::beast::string_view content_type);
        FileResponse GETMakeFileResponse(http::status status, fs::path file_path, unsigned http_version, bool keep_alive, std::string content_type);
        FileResponse HEADMakeFileResponse(http::status status, fs::path file_path, unsigned http_version, bool keep_alive, std::string content_type);

        void SetFilePath(fs::path file_path);

        bool CheckAuthorization(const app::Token& tocken);

        //Выполнение какого-либо функционала, для которого необходим токен
        template <typename Body, typename Allocator, typename Send, typename Fn >
        void API_PerfomActionWithToken(const http::request<Body, http::basic_fields<Allocator>>& req, Send& send, Fn&& func) {
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
                return;
            }
            auto token_from_req = auth.substr(7);
            if (CheckAuthorization(token_from_req)) {
                auto answer = func(token_from_req);
                if (answer == ErrorParseAction()){
                    auto response = json_text_response(std::move(answer), http::status::bad_request);
                    send(response);
                    return;
                }
               auto response = json_text_response(std::move(answer), http::status::ok);
               send(response);
               return;
            }
            else {
                auto response = json_text_response(PlayerNotFound(), http::status::unauthorized);
                send(response);
                return;
            }
        }



        template <typename Body, typename Allocator, typename Send>
        void GetAPI_info_RequestHand(const http::request<Body, http::basic_fields<Allocator>>& req, Send& send) {
            const auto text_response = [&req, this](http::status status, std::string_view text, boost::beast::string_view content_type) {
               auto response  = this->MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type);
               response.set(http::field::cache_control, "no-cache");
               return response;
                };
            const auto json_text_response = [&req, this](json::value&& jv, http::status status) {
                std::string answ = json::serialize(jv);
                StringResponse response = MakeStringResponse(status, answ, req.version(), req.keep_alive(), "application/json");
                response.set(http::field::cache_control, "no-cache");
                return response;
                };
            if ((req.method_string() == "GET") &&
                (static_cast<std::string>(req.target()) == "/api/v1/maps"))
            {
                std::string answ = GetMaps(game_);
                auto response = text_response(http::status::ok, answ, "application/json");
                send(response);
                return;
            }
            if ((req.method_string() == "GET" || req.method_string() == "HEAD") &&
                (static_cast<std::string>(req.target()).substr(0, 13) == "/api/v1/maps/"))
            {
                std::string id = static_cast<std::string>(req.target()).substr(13); //получаем номер карты
                auto map = game_.FindMap(model::Map::Id(id)); //ищем карту
                if (map) {
                    //std::string answ = json::serialize(json::value_from(*map));
                    std::string answ = json::serialize(json::value_from(std::pair<model::Map, boost::json::array>(*map, lost_objects_json_data_.Get(id))));
                    auto response = text_response(http::status::ok, answ, "application/json");
                    send(response);
                    return;
                }
                else {
                    std::string answ = json::serialize(MapNotFound());  //Карты не нашлось
                    auto response = text_response(http::status::not_found, answ, "application/json");
                    send(response);
                    return;
                }
            }

            if ((static_cast<std::string>(req.target()).substr(0, 13) == "/api/v1/maps/")) {
                auto response = json_text_response(InvalidMethod(), http::status::method_not_allowed);
                response.set(http::field::allow, "GET, HEAD");
                send(response);
                return;
            }
            return;
        }
        template <typename Body, typename Allocator, typename Send>
        bool GetStaticFile_RequestHand(const http::request<Body, http::basic_fields<Allocator>>& req, Send& send) {
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
                        return true;
                    }
                    std::string file_type = GetFileType(file_path.string());
                    if (req.method_string() == "GET") {
                        auto response = get_file_response(http::status::ok, file_path, file_type);
                        send(response);
                        return true;
                    }
                    if (req.method_string() == "HEAD") {
                        auto response  = head_file_response(http::status::ok, file_path, file_type);
                        send(response);
                        return true;
                    }
                }
                else {
                    auto response = text_response(http::status::bad_request, "Not access", "text/plain");
                    send(response);
                    return true;
                }
            }
            return false;
        }
        template <typename Body, typename Allocator, typename Send>
        void API_AuthGame_RequestHand(const http::request<Body, http::basic_fields<Allocator>>& req, Send& send) {
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
                    return;
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
                    return;
                }
                //ошибка пустого имени
                if (userName == "") {
                    auto response = json_text_response(EmptyNickname(), http::status::bad_request);
                    send(response);
                    return;
                }

                //ошибка отсутствия карты
                if (!game_.FindMap(model::Map::Id(static_cast<std::string>(mapId)))) {
                    auto response = json_text_response(MapNotFound(), http::status::not_found);
                    send(response);
                    return;
               }
           
                //установка игрока
                boost::asio::dispatch(strand_, [req, send = std::forward<Send>(send), this, userName, mapId] () {
                const auto json_text_response = [&req, this](json::value&& jv, http::status status) {
                    std::string answ = json::serialize(jv);
                    StringResponse response = this->MakeStringResponse(status, answ, req.version(), req.keep_alive(), "application/json");
                    response.set(http::field::cache_control, "no-cache");
                    return response;
                    };
                //Поиск (а в случае неудачи создание) игровой сессии
                std::shared_ptr<model::GameSession> gs = this->game_.FindGameSession(model::Map::Id(static_cast<std::string>(mapId)));
                //Создание пса; Создание игрока для этого пса и этой игровой сессии; Создание токена для этого игрока 
                auto players_token = this->tokens_.AddPlayer(players_.Add(std::make_shared<model::Dog>(static_cast<std::string>(userName)),gs));
                gs->GenerateForced();
                json::value answer = {
                    {"authToken", players_token},
                    {"playerId", (*(this->tokens_).FindPlayerByToken(players_token)).GetDogId()}
                };
                auto response = json_text_response(std::move(answer), http::status::ok);
                send(response);
                    });
                return;
            }
            else if (static_cast<std::string>(req.target()) == "/api/v1/game/join") {
                auto response = json_text_response(NotPostRequest(), http::status::method_not_allowed);
                response.set(http::field::allow, "POST");
                send(response);
                return;
            }
            return;
        }
        template <typename Body, typename Allocator, typename Send>
        void API_PlayersList_RequestHand(const http::request<Body, http::basic_fields<Allocator>>& req, Send& send) {
            const auto json_text_response = [&req, this](json::value&& jv, http::status status) {
                std::string answ = json::serialize(jv);
                StringResponse response = MakeStringResponse(status, answ, req.version(), req.keep_alive(), "application/json");
                response.set(http::field::cache_control, "no-cache");
                return response;
                };
            if (((req.method_string() == "GET") || (req.method_string() == "HEAD"))&&
                (static_cast<std::string>(req.target()) == "/api/v1/game/players")) {
           
                boost::asio::dispatch(strand_, [send = std::forward<Send>(send), this, req]() {
                API_PerfomActionWithToken(req, send, [this](const app::Token& token) {
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
                    return answer; });
                });
            }
            else if(static_cast<std::string>(req.target()) == "/api/v1/game/players"){
                auto response = json_text_response(InvalidMethod(), http::status::method_not_allowed);
                response.set(http::field::allow, "GET, HEAD");
                send(response);
                return;
            }
            return;
        }
        template <typename Body, typename Allocator, typename Send>
        void API_GameState_RequestHand(const http::request<Body, http::basic_fields<Allocator>>& req, Send& send) {
            const auto json_text_response = [&req, this](json::value&& jv, http::status status) {
                std::string answ = json::serialize(jv);
                StringResponse response = MakeStringResponse(status, answ, req.version(), req.keep_alive(), "application/json");
                response.set(http::field::cache_control, "no-cache");
                return response;
                };
            if (((req.method_string() == "GET") || (req.method_string() == "HEAD")) &&
                (static_cast<std::string>(req.target()) == "/api/v1/game/state")) {

                boost::asio::dispatch(strand_, [send = std::forward<Send>(send), this, req]() {
                API_PerfomActionWithToken(req, send, [this](const app::Token& token) {
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
                    json::value information_about_lost_object;
                    json::value lost_objects;
                    if (gs->GetCurrentLostObjects().size() != 0) {

                        information_about_lost_object = {
                            {"type", gs->GetCurrentLostObjects().begin()->type},
                            {"pos", std::vector<double>({gs->GetCurrentLostObjects().begin()->position.x,gs->GetCurrentLostObjects().begin()->position.y})}
                        };
                        lost_objects = {
                        {std::to_string(0), information_about_lost_object}
                        };
                        int k = 1;
                        for (auto p = (gs->GetCurrentLostObjects().begin() + 1); p != gs->GetCurrentLostObjects().end(); ++p) {
                            information_about_lost_object = {
                                {"type", p->type},
                                {"pos", {p->position.x, p->position.y}}
                            };
                            lost_objects.get_object().emplace(std::to_string(k++), information_about_lost_object);
                        }
                    }

                    json::value answer = {
                        { "players", players },
                        {"lostObjects", lost_objects }
                    };
                    return answer; }); 
                });
            }
             else if (static_cast<std::string>(req.target()) == "/api/v1/game/state") {
                auto response = json_text_response(InvalidMethod(), http::status::method_not_allowed);
                response.set(http::field::allow, "GET, HEAD");
                send(response);
                return;           
             }
             return;
        }
        template <typename Body, typename Allocator, typename Send>
        void API_MovePlayer_RequestHand(const http::request<Body, http::basic_fields<Allocator>>& req, Send& send) {
            const auto json_text_response = [&req, this](json::value&& jv, http::status status) {
                std::string answ = json::serialize(jv);
                StringResponse response = MakeStringResponse(status, answ, req.version(), req.keep_alive(), "application/json");
                response.set(http::field::cache_control, "no-cache");
                return response;
                };

            if ((req.method_string() == "POST") &&
                (static_cast<std::string>(req.target()) == "/api/v1/game/player/action")) {
                try {
                    if (req.base()["Content-Type"].to_string() != "application/json")
                        throw std::exception();
                }
                catch (...) {
                    auto response = json_text_response(InvalidContentType(), http::status::bad_request);
                    send(response);
                    return;
                }
                boost::asio::dispatch(strand_, [send = std::forward<Send>(send), this, req]() {
                API_PerfomActionWithToken(req, send, [this, &req](const app::Token& token) {
                    auto player = this->tokens_.FindPlayerByToken(token);
                    double dog_speed = this->tokens_.FindPlayerByToken(token)->GetGameSession()->GetDogSpeed();

                    json::error_code ec;
                    json::value jv = json::parse(req.body(), ec);
                    if (ec || !jv.as_object().contains("move"))
                        return ErrorParseAction();
                
                    std::string dir = static_cast<std::string>(jv.as_object().at("move").as_string());
                   // std::lock_guard lg(m);
                    if (dir == "L") {
                        player->SetDogSpeed(-dog_speed, 0.);
                        player->SetDogDirection(dir);
                    }
                    else if (dir == "R") {
                        player->SetDogSpeed(dog_speed, 0.);
                        player->SetDogDirection(dir);
                    }
                    else if (dir == "U") {
                        player->SetDogSpeed(0., -dog_speed);
                        player->SetDogDirection(dir);
                    }
                    else if (dir == "D") {
                        player->SetDogSpeed(0., dog_speed);
                        player->SetDogDirection(dir);
                    }
                    else if (dir == "") {
                        player->SetDogSpeed(0., 0.);
                        player->SetDogDirection("U");
                    }
                    else {
                        return ErrorParseAction();
                    }
                    json::value answer = json::object();
                    return answer;});
                });

            }
            else if (static_cast<std::string>(req.target()) == "/api/v1/game/player/action") {
                auto response = json_text_response(NotPostRequest(), http::status::method_not_allowed);
                response.set(http::field::allow, "POST");
                send(response);
                return;
            }
            return;
        }
        template <typename Body, typename Allocator, typename Send>
        void API_TimeTick_RequestHand(const http::request<Body, http::basic_fields<Allocator>>& req, Send& send) {
            const auto json_text_response = [&req, this](json::value&& jv, http::status status) {
                std::string answ = json::serialize(jv);
                StringResponse response = MakeStringResponse(status, answ, req.version(), req.keep_alive(), "application/json");
                response.set(http::field::cache_control, "no-cache");
                return response;
                };

            if ((req.method_string() == "POST") &&
                (static_cast<std::string>(req.target()) == "/api/v1/game/tick")) {
                json::error_code ec;
                json::value jv = json::parse(req.body(), ec);
                if (ec || !jv.as_object().contains("timeDelta")) {
                    auto response = json_text_response(ErrorParseTick(), http::status::bad_request);
                    send(response);
                    return;
                }
                if (!jv.as_object().at("timeDelta").if_int64()) {
                    auto response = json_text_response(ErrorParseTick(), http::status::bad_request);
                    send(response);
                    return;
                }
            

                boost::asio::dispatch(strand_, [send = std::forward<Send>(send), this, jv, req]() {
                const auto json_text_response = [&req, this](json::value&& jv, http::status status) {
                    std::string answ = json::serialize(jv);
                    StringResponse response = this->MakeStringResponse(status, answ, req.version(), req.keep_alive(), "application/json");
                    response.set(http::field::cache_control, "no-cache");
                    return response;
                    };
                double time_delta = jv.as_object().at("timeDelta").as_int64(); //время в миллисикундах
                //m.lock();
                this->players_.MoveAllDogs(time_delta);
                //m.unlock();
                json::value answer = json::object();
                auto response = json_text_response(std::move(answer), http::status::ok);
                send(response);
                });
                return;
            }
            else if (static_cast<std::string>(req.target()) == "/api/v1/game/tick") {
                auto response = json_text_response(NotPostRequest(), http::status::method_not_allowed);
                response.set(http::field::allow, "POST");
                send(response);
                return;
            }
            return;
        }

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            const auto text_response = [&req, this](http::status status, std::string_view text, boost::beast::string_view content_type) {
                return this->MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type);
            };
 
            if (static_cast<std::string>(req.target()) == "/api/v1/maps" || static_cast<std::string>(req.target()).substr(0, 13) == "/api/v1/maps/") {
                GetAPI_info_RequestHand(req, send);
                return;
            }
            if (GetStaticFile_RequestHand(req, send))
                return;
            if (static_cast<std::string>(req.target()) == "/api/v1/game/join") {
                API_AuthGame_RequestHand(req, send);
                return;
            }
            if (static_cast<std::string>(req.target()) == "/api/v1/game/players") {
                API_PlayersList_RequestHand(req, send);
                return;
            }
            if (static_cast<std::string>(req.target()) == "/api/v1/game/state") {
                API_GameState_RequestHand(req, send);
                return;
            }
            if (static_cast<std::string>(req.target()) == "/api/v1/game/player/action") {
                API_MovePlayer_RequestHand(req, send);
                return;
            }
            if (static_cast<std::string>(req.target()) == "/api/v1/game/tick" && !IsAutomaticTick) {
                API_TimeTick_RequestHand(req, send);
                return;
            }

           std::string answ = json::serialize(BadRequest());   //Невалидный запрос
           auto response = text_response(http::status::bad_request, answ, "application/json");
           response.set(http::field::cache_control, "no-cache");
           send(response);
           return;
        }

        void Tick(std::chrono::milliseconds time_delta) {
            boost::asio::dispatch(strand_, [this, time_delta]() {
            this->players_.MoveAllDogs(std::chrono::duration<double>(time_delta).count() * 1000);
            for (int i = 0; i < this->game_.GetGameSessions().size(); ++i)
                this->game_.GetGameSessions()[i]->GenerateLoot(time_delta);
            });
        
        }
        void SetAutomaticTick() {
            IsAutomaticTick = true;
        }

    private:
        model::Game& game_;
        app::Players players_;
        app::PlayerTokens tokens_;
        extra_data::Json_data lost_objects_json_data_;
        bool IsAutomaticTick = false;

        std::filesystem::path path_;

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
         template <typename t>
         void LogResponse(t&& res, std::string content_type, int time_of_making_response) {
             json::value jv;
                 jv = {
                      {"response_time", time_of_making_response},
                      {"code", res.result_int()},
                      {"content_type", content_type}
                      };
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

                 request_handler_(std::move(req), [send = std::forward<Send>(send), this, t1](auto&& response) {
                     std::string content_type = response.base()["Content-Type"].to_string();
                     send(response);
                     auto t2 = clock();
                     this->LogResponse(response, content_type, int(t2 - t1));
                 });

             }
         }

     private:

         RequestHandlerType& request_handler_;
     };
}  // namespace http_handler
