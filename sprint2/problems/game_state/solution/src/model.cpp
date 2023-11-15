#include "model.h"

#include <stdexcept>

namespace model {
using namespace std::literals;

void Map::AddOffice(const Office& office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(office);
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
         // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Game::AddMap(const Map& map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(map);
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Road& road)
{

    if (road.IsHorizontal())
        jv = {
            {"x0", road.GetStart().x},
            {"y0", road.GetStart().y},
            {"x1", road.GetEnd().x}
    };
    if (road.IsVertical())
        jv = {
            {"x0", road.GetStart().x},
            {"y0", road.GetStart().y},
            {"y1", road.GetEnd().y}
    };
}

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Building& build) {
    jv = {
        {"x", build.GetBounds().position.x},
        {"y", build.GetBounds().position.y},
        {"w", build.GetBounds().size.width},
        {"h", build.GetBounds().size.height}
    };
}

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Office& office) {
    jv = {
        {"id", *office.GetId()},
        {"x", office.GetPosition().x},
        {"y", office.GetPosition().y},
        {"offsetX", office.GetOffset().dx},
        {"offsetY", (int)office.GetOffset().dy}
    };
}
void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Map& map) {
    jv = {
        {"id", *map.GetId()},
        {"name", map.GetName()},
        {"roads", map.GetRoads()},
        {"buildings", map.GetBuildings()},
        {"offices", map.GetOffices()}
    };
}
void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, Game const& game) {
    jv = {
        {"maps", game.GetMaps()}
    };
}

}  // namespace model
