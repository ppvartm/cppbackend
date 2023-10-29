#include "json_loader.h"


namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path) {

    model::Game game;

    std::ifstream file;
    file.open(json_path);

    std::stringstream ss;
    ss << file.rdbuf();
    std::string s = ss.str();

    boost::json::error_code ec;
    auto value = boost::json::parse(s, ec);
   
    auto maps = value.as_object().at("maps").as_array(); 

    for (int j = 0; j < maps.size(); ++j) {

        auto js_id = maps.at(j).as_object().at("id").as_string();
        auto js_name = maps.at(j).as_object().at("name").as_string();
        model::Map::Id id{ static_cast<std::string>(js_id) };
        model::Map map(id, static_cast<std::string>(js_name));   //инициализация карты

        auto roads = maps.at(j).as_object().at("roads").as_array();
        for (int i = 0; i < roads.size(); ++i) {
            model::Point start = { roads[i].as_object().at("x0").as_int64() , roads[i].as_object().at("y0").as_int64() };
            if (roads[i].as_object().contains("x1")) {
                model::Coord end_x = roads[i].as_object().at("x1").as_int64();
                map.AddRoad(model::Road(model::Road::HORIZONTAL, start, end_x));
            }
            else {
                model::Coord end_y = roads[i].as_object().at("y1").as_int64();
                map.AddRoad(model::Road(model::Road::VERTICAL, start, end_y));
            }
        }

        auto buildings = maps.at(j).as_object().at("buildings").as_array();
        for (int i = 0; i < buildings.size(); ++i) {
            model::Point position = { buildings[i].as_object().at("x").as_int64(),buildings[i].as_object().at("y").as_int64() };
            model::Size size = { buildings[i].as_object().at("w").as_int64(), buildings[i].as_object().at("h").as_int64() };
            map.AddBuilding(model::Building({ position, size }));
        }

        auto offices = maps.at(j).as_object().at("offices").as_array();
        for (int i = 0; i < offices.size(); i++) {
            model::Office::Id id{ static_cast<std::string>(offices[i].as_object().at("id").as_string()) };
            model::Point position = { offices[i].as_object().at("x").as_int64(),offices[i].as_object().at("y").as_int64() };
            model::Offset offset = { offices[i].as_object().at("offsetX").as_int64(), offices[i].as_object().at("offsetY").as_int64() };
            map.AddOffice(model::Office({ id, position, offset }));
        }

        game.AddMap(map);
    }
    return game;
}

}  // namespace json_loader
