#include "json_loader.h"


namespace json_loader {

    model::Map MakeMap(const json::object& json_map) {
        auto js_id = json_map.at("id").as_string();
        auto js_name = json_map.at("name").as_string();
        model::Map::Id id{ static_cast<std::string>(js_id) };
        return model::Map(id, static_cast<std::string>(js_name));
    }

    void AddRoads(const json::object& json_map, model::Map& map) {
        auto roads = json_map.at("roads").as_array();
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
    }
    void AddBuildings(const json::object& json_map, model::Map& map) {
        auto buildings = json_map.at("buildings").as_array();
        for (int i = 0; i < buildings.size(); ++i) {
            model::Point position = { buildings[i].as_object().at("x").as_int64(),buildings[i].as_object().at("y").as_int64() };
            model::Size size = { buildings[i].as_object().at("w").as_int64(), buildings[i].as_object().at("h").as_int64() };
            map.AddBuilding(model::Building({ position, size }));
        }
    }
    void AddOffices(const json::object& json_map, model::Map& map) {
        auto offices = json_map.at("offices").as_array();
        for (int i = 0; i < offices.size(); i++) {
            model::Office::Id id{ static_cast<std::string>(offices[i].as_object().at("id").as_string()) };
            model::Point position = { offices[i].as_object().at("x").as_int64(),offices[i].as_object().at("y").as_int64() };
            model::Offset offset = { offices[i].as_object().at("offsetX").as_int64(), offices[i].as_object().at("offsetY").as_int64() };
            map.AddOffice(model::Office({ id, position, offset }));
        }
    }

    void SetDogSpeed(const json::object& json_map, model::Map& map) {
        if (json_map.contains("dogSpeed"))
            map.SetDogSpeed(json_map.at("dogSpeed").as_double());
    }

    model::Game LoadGame(std::filesystem::path json_path) {

        model::Game game;
        json_path = std::filesystem::weakly_canonical(json_path);
        std::ifstream file;
        if(std::filesystem::exists(json_path))
          file.open(json_path);
        else 
            throw("Json doesn't exist");
        
        std::stringstream ss;
        ss << file.rdbuf();
        std::string s = ss.str();

        json::error_code ec;
        auto value = json::parse(s, ec);
        if (ec)
            throw("Failed to parse json: " + ec.message());
   
        auto maps = value.as_object().at("maps").as_array(); 
        
        bool is_defaultDogSpeed_exist = false;
        model::DogSpeedFromJson defaultDogSpeed = 0;
        if (value.as_object().contains("defaultDogSpeed")) {
            is_defaultDogSpeed_exist = true;
            defaultDogSpeed = value.as_object().at("defaultDogSpeed").as_double();
        }

        for (int j = 0; j < maps.size(); ++j) {

            model::Map map = MakeMap(maps.at(j).as_object());
            if (is_defaultDogSpeed_exist)
                map.SetDogSpeed(defaultDogSpeed);

            AddRoads(maps.at(j).as_object(), map);
            AddBuildings(maps.at(j).as_object(), map);
            AddOffices(maps.at(j).as_object(), map);
            SetDogSpeed(maps.at(j).as_object(), map);
            game.AddMap(map);
          
            
        }
        return game;
    }

}  // namespace json_loader
