#pragma once
#include <memory>
#include <random>
#include <unordered_map>

#include <boost/functional/hash.hpp>

#include "../model/model.h"

namespace app {
    using Token = std::string;

    //move the dog; associated with certain dog and certain game-session
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
        std::vector<model::Road> GetRoadsWithDog();
        model::Position NewCorrectPosition(model::Position new_position);
        void MoveDog(double time_delta);
	private:
        std::shared_ptr<model::Dog> dog_;
		std::shared_ptr<model::GameSession> game_session_;		
	};
    //contains the pairs player-tocken 
    class PlayerTokens {  
    public:
        Token AddPlayer(std::shared_ptr<Player> player) {
            Token token = GenerateToken();
            token_to_player_[token] = player;
            return token;
        }
        std::shared_ptr<Player> FindPlayerByToken(const Token& token) {
            auto player = token_to_player_.find(token);
            if (player != token_to_player_.end())
                return player->second;
            return nullptr;

        }
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
    };
    //contains the all players
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