#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <atomic>

#include <boost/json.hpp>

#include "loot_generator.h"
#include "tagged.h"
#include "collision_detector.h"

namespace model {


    using Dimension = long long int;
    using Coord = Dimension;
    using DogSpeedFromJson = double;
    

    enum class Direction { NORTH, SOUTH, WEST, EAST };

    struct LootGeneratorParams {
        double period;
        double probability;
    };
    struct Position {
        double x;
        double y;
    };
    struct Speed {
        double s_x;
        double s_y;
    };
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
    struct LostObject {
        int type;
        Position position;
        int value;
    };
    struct GatheringEvent {
        size_t dog_id;
        size_t lost_object_id;
        double time;
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
            : start_{ start }
            , end_{ end_x, start.y } {
        }

        Road(VerticalTag, Point start, Coord end_y) noexcept
            : start_{ start }
            , end_{ start.x, end_y } {
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
            : bounds_{ bounds } {
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
            : id_{ std::move(id) }
            , position_{ position }
            , offset_{ offset } {
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

        void SetDogSpeed(DogSpeedFromJson dog_speed) {
            dog_speed_ = dog_speed;
        }

        const DogSpeedFromJson GetDogSpeed() const {
            return dog_speed_;
        }

        void SetMaxCountOfLootObjects(unsigned num) {
            max_count_of_loot_objects_ = num;
        }

        int GetMaxCountOfLootObjects() const {
            return max_count_of_loot_objects_;
        }

        std::map<int, int>& GetPriceList()  {
            return price_list_;
        }

        const std::map<int, int>& GetPriceList() const {
            return price_list_;
        }

        void SetBagCapacity(unsigned cap) {
            bag_capacity_ = cap;
        }

        size_t GetBagCapacity() const {
            return bag_capacity_;
        }
    private:
        using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

        Id id_;
        std::string name_;
        Roads roads_;
        Buildings buildings_;

        OfficeIdToIndex warehouse_id_to_index_;
        Offices offices_;

        unsigned max_count_of_loot_objects_;
        std::map<int, int> price_list_;

        DogSpeedFromJson dog_speed_ = 1.;


        unsigned bag_capacity_;
    };

    class Bag {
    public:
        void SetCapacity(size_t cap) {
            capacity_ = cap;
        }
        bool AddObject(size_t id, const LostObject& obj_in_bag) {
            if (objects_in_bag_.size() <= capacity_) {
                objects_in_bag_[id] = obj_in_bag;
                return true;
            }
            return false;
        }
        void RemoveObjects() {
            objects_in_bag_.clear();
        }
        bool IsFull() const {
            return objects_in_bag_.size() >= capacity_;
        }
        const std::map<size_t, LostObject>& Get() const {
            return objects_in_bag_;
        }
        int GetValues() const {
            int value = 0;
            for (auto& p : objects_in_bag_)
                value += p.second.value;
            return value;
        }
    private:
        size_t capacity_;
        std::map<size_t, LostObject> objects_in_bag_;
    };

    class Dog {
    public:
        using Id = uint64_t;
        Dog(std::string name) :speed_({ 0,0 }), dir_(Direction::NORTH), name_(name), id_(general_id_++) {
        };
        void SetPosition(Position pos) {
            pos_ = pos;
        }
        const Id GetId() const {
            return id_;
        }
        const std::string& GetName() const {
            return name_;
        }
        const Position GetPosition() const {
            return pos_;
        }
        const Speed GetSpeed() const {
            return speed_;
        }
        const Direction GetDirection() const {
            return dir_;
        }
        const std::string GetDirectionToString() const {
            Direction temp = GetDirection();
            if (temp == Direction::NORTH)
                return "U";
            if (temp == Direction::SOUTH)
                return "D";
            if (temp == Direction::WEST)
                return "L";
            if (temp == Direction::EAST)
                return "R";
        }
        void SetDirection(std::string dir) {
            if (dir == "U")
                dir_ = Direction::NORTH;
            if (dir == "D")
                dir_ = Direction::SOUTH;
            if (dir == "L")
                dir_ = Direction::WEST;
            if (dir == "R")
                dir_ = Direction::EAST;
        }
        void SetSpeed(Speed speed) {
            speed_ = speed;
        }

        void SetBagCapacity(size_t cap) {
            bag_.SetCapacity(cap);
        }
        bool AddObjectInBag(size_t id, const LostObject& lost_object) {
            return bag_.AddObject(id, lost_object);
        }
        void RemoveObjectsFromBag() {
            bag_.RemoveObjects();
        }
        bool IsFullBag() const {
            return bag_.IsFull();
        }
        const std::map<size_t, LostObject>& GetBag() const {
            return bag_.Get();
        }

        Position GetNextPosition(double time_delta) {
            return { GetPosition().x + GetSpeed().s_x * time_delta / 1000.,  GetPosition().y + GetSpeed().s_y * time_delta / 1000. };
        }
        
        void AddScore() {
            score_ += bag_.GetValues();
        }
        int GetScore() const {
            return score_;
        }
    private:
        Position pos_ = { 0, 0 };
        Speed speed_;
        Direction dir_;

        std::string name_;
        Id id_;

        static inline Id general_id_ = 0;

        Bag bag_;
        int score_ = 0;
    };

    //Хранит карту и набор собак на ней
    class GameSession {
    public:
        GameSession(const Map& map, bool is_rand_spawn, LootGeneratorParams loot_generator_params) :
            map_(map), is_rand_spawn_(is_rand_spawn),
            loot_generator_(static_cast<int>(loot_generator_params.period * 1000) * 1ms, loot_generator_params.probability) {}
        Map::Id GetMapId() const {
            return map_.GetId();
        }
        void AddDog(std::shared_ptr<Dog> dog);
        const std::multimap<std::string, std::shared_ptr<Dog>>& GetDogs() const {
            return dogs_;
        }
        const DogSpeedFromJson GetDogSpeed() const {
            return map_.GetDogSpeed();
        }
        const Map& GetMap() const {
            return map_;
        }

        const std::map<size_t, LostObject>& GetCurrentLostObjects() const {
            return lost_objects_;
        }

        void PushLostObject(const LostObject& lost_object) {
            lost_objects_[lost_object_id_++] = lost_object;
        }
        void GenerateLoot(std::chrono::milliseconds time_delta) {
            unsigned need_to_gen = loot_generator_.Generate(time_delta, GetCurrentLostObjects().size(), dogs_.size());
            srand(time(NULL));
            for (unsigned i = 0; i < need_to_gen; ++i) {
                int type = rand() % map_.GetMaxCountOfLootObjects();
                int value = map_.GetPriceList().find(type)->second;
                Position pos;
                auto road = map_.GetRoads()[rand() % map_.GetRoads().size()];
                if (road.IsVertical() && road.GetEnd().y > road.GetStart().y) {
                    pos.x = road.GetStart().x;
                    pos.y = (rand() % (road.GetEnd().y - road.GetStart().y)) + road.GetStart().y;
                }
                if (road.IsVertical() && road.GetEnd().y < road.GetStart().y) {
                    pos.x = road.GetStart().x;
                    pos.y = (rand() % (road.GetStart().y - road.GetEnd().y)) + road.GetEnd().y;
                }               
                if (road.IsHorizontal() && road.GetEnd().x > road.GetStart().x){
                    pos.y = road.GetStart().y;
                    pos.x = (rand() % (road.GetEnd().x - road.GetStart().x)) + road.GetStart().x;
                }
                if (road.IsHorizontal() && road.GetEnd().x < road.GetStart().x) {
                    pos.y = road.GetStart().y;
                    pos.x = (rand() % (road.GetStart().x- road.GetEnd().x)) + road.GetEnd().x;
                }
                PushLostObject({ type, pos, value });
            }
        }
        void GenerateForced() {
            int type = rand() % map_.GetMaxCountOfLootObjects();
            Position pos;
            int value = map_.GetPriceList().find(type)->second;
            auto road = map_.GetRoads()[rand() % map_.GetRoads().size()];
            if (road.IsVertical() && road.GetEnd().y > road.GetStart().y) {
                pos.x = road.GetStart().x;
                pos.y = (rand() % (road.GetEnd().y- road.GetStart().y)) + road.GetStart().y;
            }
            if (road.IsVertical() && road.GetEnd().y < road.GetStart().y) {
                pos.x = road.GetStart().x;
                pos.y = (rand() % (road.GetStart().y - road.GetEnd().y)) + road.GetEnd().y;
            }
            if (road.IsHorizontal() && road.GetEnd().x > road.GetStart().x) {
                pos.y = road.GetStart().y;
                pos.x = (rand() % (road.GetEnd().x - road.GetStart().x)) + road.GetStart().x;
            }
            if (road.IsHorizontal() && road.GetEnd().x < road.GetStart().x) {
                pos.y = road.GetStart().y;
                pos.x = (rand() % (road.GetStart().x - road.GetEnd().x)) + road.GetEnd().x;
            }
            PushLostObject({ type, pos, value});
        }
        Dog& GetDog(size_t id) const {
            auto dog = dogs_.begin();
            for (int i = 0; i < id; ++i)
                ++dog;
            return *(dog->second);
        }
        std::pair<size_t, LostObject> GetLostObject(size_t id) const {
            auto lost_object = lost_objects_.begin();
            for (int i = 0; i < id; ++i)
                ++lost_object;
            return *lost_object;
        }

        void CollectionItems(double time_delta) {
            std::vector<GatheringEvent> result;
            auto lost_objects = lost_objects_;
            for (size_t i = 0; i < lost_objects.size(); ++i) {
                auto item = GetLostObject(i);
                for (size_t g = 0; g < dogs_.size(); ++g) {
                    auto gatherer = GetDog(g);
                    auto collection_result = collision_detector::TryCollectPoint(
                        { gatherer.GetPosition().x, gatherer.GetPosition().y },
                        { gatherer.GetNextPosition(time_delta).x,gatherer.GetNextPosition(time_delta).y },
                        { item.second.position.x, item.second.position.y });
                    if (collection_result.IsCollected(0.3)) {
                        if (!gatherer.IsFullBag())
                            result.push_back({ g ,item.first, collection_result.proj_ratio });
                    }
                }
                if (!result.empty()) {
                    std::sort(result.begin(), result.end(), [](const GatheringEvent& e_l, const GatheringEvent& e_r) {
                        return e_l.time < e_r.time;
                        });
                    GetDog(result.begin()->dog_id).AddObjectInBag(item.first, item.second);
                    lost_objects_.erase(item.first);
                    result.clear();
                }
            }

        }
        void LeaveItems(double time_delta) {
            for (size_t g = 0; g < dogs_.size(); ++g) {
                auto gatherer = GetDog(g);
                for (size_t i = 0; i < map_.GetOffices().size(); ++i) {
                    auto collection_result = collision_detector::TryCollectPoint(
                        { gatherer.GetPosition().x, gatherer.GetPosition().y },
                        { gatherer.GetNextPosition(time_delta).x,gatherer.GetNextPosition(time_delta).y },
                        {static_cast<double>(map_.GetOffices()[i].GetPosition().x), static_cast<double>(map_.GetOffices()[i].GetPosition().y) });
                    if (collection_result.IsCollected(0.55)) {
                        gatherer.AddScore();
                        gatherer.RemoveObjectsFromBag();
                    }
                }
            }
        }

    private:
        std::multimap<std::string, std::shared_ptr<Dog>> dogs_;
        std::map<size_t, LostObject> lost_objects_;

        const Map& map_;


        loot_gen::LootGenerator loot_generator_;
        bool is_rand_spawn_ = false;

        static inline std::atomic_int lost_object_id_ = 0;
    };

    //Хранит информацию о картах и набор игровых сессий
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

        std::shared_ptr<GameSession> FindGameSession(const Map::Id& id) {
            if (!FindMap(id)) return nullptr;
            for (auto p : game_sessions_)
                if (p->GetMapId() == id)
                    return p;
            return  game_sessions_.emplace_back(std::make_shared<GameSession>(*FindMap(id), is_rand_game_spawn_, loot_generator_params_));
        }
        void SetRandomSpawn() {
            is_rand_game_spawn_ = true;
        }

        void SetLootGeneratorParams(double period, double probability) {
            loot_generator_params_.period = period;
            loot_generator_params_.probability = probability;
        }
        const GameSessions& GetGameSessions() const {
            return game_sessions_;
        }

    private:
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        GameSessions game_sessions_;
        Maps maps_;
        MapIdToIndex map_id_to_index_;
        LootGeneratorParams loot_generator_params_;
        bool is_rand_game_spawn_ = false;
    };

    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Road& road);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Building& build);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Office& office);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Map& map);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Game& game);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const std::pair<Map, boost::json::array>& map_and_json_data);
    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const std::pair<size_t, LostObject>& lost_object);
}  // namespace model
