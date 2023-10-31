#include "request_handler.h"




namespace http_handler {
    

    json::value MapNotFound() {
        json::value val = {
                    {"code", "mapNotFound"},
                    {"message","Map not found"} };
        return val;
    }
    json::value BadRequest() {
        json::value val = {
                   {"code", "badRequest"},
                   {"message","Bad request"} };
        return val;
    }
    std::string GetMaps(const model::Game& game) {
        std::vector<MapInfo> maps_info;
        auto maps = game.GetMaps();
        for (int i = 0; i < maps.size(); ++i)
            maps_info.emplace_back(maps[i]);
        std::string answ = json::serialize(json::value_from(maps_info));
        return answ;
    }

    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, MapInfo const& map_info) {
        jv = {
            {"id", *map_info.id_},
            {"name", map_info.name_}
        };
    }

}   // namespace http_handler
