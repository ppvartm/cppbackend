#pragma once
#include <memory>
#include <random>
#include <unordered_map>
#include "model.h"
#include <iostream>
#include <boost/functional/hash.hpp>

namespace app {
    using Token = std::string;


	class Player {
	public:
        Player() = delete;
        Player(std::shared_ptr<model::Dog> dog, std::shared_ptr<model::GameSession> game_session):
            dog_(dog), 
            game_session_(game_session){
            game_session->AddDog(dog_);
        }

        model::Dog::Id GetDogId() {
            return dog_->GetId();
        }
        const std::shared_ptr<model::GameSession> GetGameSession() const {
            return game_session_;
        }
        void SetDogSpeed(model::DogSpeedFromJson speed_horizontal, model::DogSpeedFromJson speed_vertical) {
            dog_->SetSpeed({speed_horizontal, speed_vertical});
        }
        void SetDogDirection(const std::string& dir) {
            dog_->SetDirection(dir);
        }
        std::vector<model::Road> GetRoadsWithDog() {
            std::vector<model::Road> valid_roads;
            for (auto& road : game_session_->GetMap().GetRoads()) {
                if (road.IsHorizontal()) {
                    if ((dog_->GetPosition().x >= road.GetStart().x - 0.4 && dog_->GetPosition().y >= road.GetStart().y - 0.4) &&
                        (dog_->GetPosition().x <= road.GetEnd().x + 0.4 && dog_->GetPosition().y <= road.GetEnd().y + 0.4))
                        valid_roads.push_back(road);
                }
                else if (road.IsVertical()) {
                    if ((dog_->GetPosition().x >= road.GetStart().x - 0.4 && dog_->GetPosition().y >= road.GetStart().y - 0.4) &&
                        (dog_->GetPosition().x <= road.GetEnd().x + 0.4 && dog_->GetPosition().y <= road.GetEnd().y + 0.4))
                        valid_roads.push_back(road);
                }
            }
                    return valid_roads;
            
        }
        model::Position NewCorrectPosition(model::Position new_position) {
            auto roads = GetRoadsWithDog();
            auto answer = new_position;
            for (auto& road : roads) {
                if (road.IsHorizontal()) {
                    if (new_position.x < road.GetStart().x - 0.4)
                        answer.x = road.GetStart().x - 0.4;
                    if (new_position.x > road.GetEnd().x + 0.4)
                        answer.x = road.GetEnd().x + 0.4;
                    if (new_position.y > road.GetStart().y + 0.4)
                        answer.y = road.GetStart().y + 0.4;
                    if (new_position.y < road.GetStart().y - 0.4)
                        answer.y = road.GetStart().y - 0.4;
                }
                else {
                    if (new_position.y < road.GetStart().y - 0.4)
                        answer.y = road.GetStart().y - 0.4;
                    if (new_position.y > road.GetEnd().y + 0.4)
                        answer.y = road.GetEnd().y + 0.4;
                    if (new_position.x > road.GetStart().x + 0.4)
                        answer.x = road.GetStart().x + 0.4;
                    if (new_position.x < road.GetStart().x - 0.4)
                        answer.x = road.GetStart().x - 0.4;
                }
                if ((answer.x == new_position.x) && (answer.y == new_position.y))
                    return new_position;
                if (roads.size() == 1)
                    return answer;
                else if (&road == &(*roads.begin()))
                    answer = new_position;
            }
            return answer;
        }
      

        void MoveDog(double time_delta) {  //время в миллисикундах
            model::Position new_position = {
                dog_->GetPosition().x + dog_->GetSpeed().s_x * time_delta / 1000.,
                dog_->GetPosition().y + dog_->GetSpeed().s_y * time_delta / 1000.
            };
            model::Position new_correct_position = NewCorrectPosition(new_position);
            if((new_position.x != new_correct_position.x) || (new_position.y != new_correct_position.y))
               SetDogSpeed(0.,0.);
            dog_->SetPosition(new_correct_position);
        }
	private:
        std::shared_ptr<model::Dog> dog_;
		std::shared_ptr<model::GameSession> game_session_;
		
	};

    class PlayerTokens {
    private:

        std::unordered_map<Token, std::shared_ptr<Player>> token_to_player_;

        std::random_device random_device_;
        std::mt19937_64 generator1_{ [this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }() };
        std::mt19937_64 generator2_{ [this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }() };

        Token GenerateToken();
    public:
        std::shared_ptr<Player> FindPlayerByToken(const Token& token) {
            auto player = token_to_player_.find(token);
            if (player != token_to_player_.end())
                return player->second;
            return nullptr;

        }


        Token AddPlayer(std::shared_ptr<Player> player) {
            Token token = GenerateToken();
            token_to_player_[token] = player;
            return token;
        }
    };

    class Players {
    public:
        std::shared_ptr<Player> Add(std::shared_ptr<model::Dog> dog, std::shared_ptr<model::GameSession> game_session) {
            return players_[{dog->GetId(), *(game_session->GetMapId())}] = std::make_shared<Player>(dog, game_session);
        }
      
        std::shared_ptr<Player> FindByDogAndMapId(model::Dog::Id dog_id, model::Map::Id map_id) {
            return players_[{dog_id, *map_id}];
        }

        void MoveAllDogs(double time_delta) {
            for (auto& player: players_)
                player.second->MoveDog(time_delta);
        }

    private:
        std::unordered_map<std::pair<uint64_t, std::string>, std::shared_ptr<Player>, boost::hash<std::pair<uint64_t, std::string>>> players_;

    };
}