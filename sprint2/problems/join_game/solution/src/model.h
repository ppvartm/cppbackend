#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <boost/json.hpp>

#include "tagged.h"

namespace model {


using Dimension = long long int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class Road {
    struct HorizontalTag {
        HorizontalTag() = default;
    };

    struct VerticalTag {
        VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(const Rectangle& bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
     Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(const Office& office);

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

class Dog {
public:
    using Id = uint64_t;
    Dog(std::string name) :name_(name), id_(general_id_++) {};
    // Dog(const Dog& dog):name_(dog.name_), id


    Id GetId() const {
        return id_;
    }
    const std::string& GetName() const {
        return name_;
    }

private:
    std::string name_;
    Id id_;

    static inline Id general_id_ = 0;

};

class GameSession {
public:

    GameSession(const Map& map):map_(map){}

    Map::Id GetMapId() const {
        return map_.GetId();
    }

    void AddDog(std::shared_ptr<Dog> dog) {
        dogs_.insert({dog->GetName(),dog});
    }

    const std::multimap<std::string, std::shared_ptr<Dog>>& GetDogs() const {
        return dogs_;
    }
   
private:
    std::multimap<std::string, std::shared_ptr<Dog>> dogs_;
    const Map& map_;
};

class Game {
public:
    using Maps = std::vector<Map>;
    using GameSessions = std::vector<std::shared_ptr<GameSession>>;

    void AddMap(const Map& map);
    const Maps& GetMaps() const noexcept {
        return maps_;
    }
    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second); 
        }
        return nullptr;
    }

    std::shared_ptr<GameSession> FindGameSession(const Map::Id& id){
        if (!FindMap(id)) return nullptr;
        for (auto p : game_sessions_)
            if (p->GetMapId() == id)
                return p;

        return  game_sessions_.emplace_back(std::make_shared<GameSession>(*FindMap(id)));
    }


private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    GameSessions game_sessions_;
    Maps maps_;
    MapIdToIndex map_id_to_index_;
};


void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Road& road);
void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Building& build);
void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Office& office);
void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Map& map);
void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Game& game);
}  // namespace model
