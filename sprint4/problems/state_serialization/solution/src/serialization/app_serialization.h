#pragma once


#include <fstream>
#include <iostream>

#include "../app/app.h"
#include "../model/model.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>



namespace app_serialization {

/*
	class SerLostObject {
	public:
		SerLostObject(const model::LostObject& lost_object): type_(lost_object.type), position_(lost_object.position), value_(lost_object.value) {
		}
		template<class Archive>
		void serialize(Archive& ar, [[maybe_unused]] const unsigned int version) {
			ar& type_;
			ar& position_.x;
			ar& position_.y;
			ar& value_;
		}

	private:
		int type_;
		model::Position position_;
		int value_;
	};

	class SerBag {
	public:
		SerBag(const model::Bag& bag) : capacity_(bag.GetCapacity()) {
			for (auto p: bag.GetLostObjects()) {
				objects_in_bag_.insert({ p.first, p.second });
			}
		}
		template<class Archive>
		void serialize(Archive& ar, [[maybe_unused]] const unsigned int version) {
			ar& capacity_;
			ar& objects_in_bag_;
		}
	private:
		size_t capacity_;
		std::map<size_t, SerLostObject> objects_in_bag_;
	};

	class SerDog {
	public:
		SerDog(const model::Dog& dog) :pos_(dog.GetPosition()), speed_(dog.GetSpeed()),
			                           dir_(dog.GetDirectionToString()), name_(dog.GetName()), id_(dog.GetId()),
		                               bag_(dog.GetBagObject()), score_(dog.GetScore()) {                                
		}

		template<class Archive>
		void serialize(Archive& ar, [[maybe_unused]] const unsigned int version) {
			ar& pos_.x;
			ar& pos_.y;
			ar& speed_.s_x;
			ar& speed_.s_y;
			ar& dir_;
			ar& name_;
			ar& id_;
			ar& bag_;
			ar& score_;
		}

	private:
		model::Position pos_;
		model::Speed speed_;
		std::string dir_;

		std::string name_;
		model::Dog::Id id_;

		SerBag bag_;
		int score_;
	};

	/*class SerGameSession {
	public:

	private:
     
	};

	class SerPlayer {
	public:
		SerPlayer(const app::Player& player): dog_(player.GetDog()) {
		}
		template <typename Archive>
		void serialize(Archive& ar, [[maybe_unused]] const unsigned int version) {
			ar& SerDog(*dog_);
		}
	private:
		std::shared_ptr<model::Dog> dog_;
	};

	

	class SerPlayerTokens {
	public:
		SerPlayerTokens(const app::PlayerTokens& player_tokens):token_to_player_(player_tokens.GetTokens()){

		}
		template <typename Archive>
		void serialize(Archive& ar, [[maybe_unused]] const unsigned int version) {
			ar& token_to_player_;
		}

	private:
		std::unordered_map<app::Token, std::shared_ptr<app::Player>> token_to_player_;
	};*/

    struct GameState{
	   std::string map_id;
	   std::vector<model::Dog> dogs;
	   std::vector<app::Token> tokens;
	   std::map<size_t, model::LostObject> lost_objects;

	   template <typename Archive>
	   void serialize(Archive& ar, [[maybe_unused]] const unsigned int file_version) {
		   ar& map_id;
		   ar& dogs;
		   ar& tokens;
		   ar& lost_objects;
	   }

    };


	class SerializingListener : public app::ApplicationListener
	{
	public:
		SerializingListener(app::Players& players, model::Game& game, app::PlayerTokens& tokens):
			app::ApplicationListener(players, game, tokens){}
		void OnTick(double time_delta) {
			if (is_save_ && is_auto_save_) {
				time_since_save_ += time_delta;
				if (time_since_save_ >= save_interval_) {
				   Serialize();
				   time_since_save_ = 0.;
				}
			}
		}

		void Serialize() {

			if (game_.GetGameSessions().size() != 0) {
				std::string map_id = *(*game_.GetGameSessions().begin())->GetMapId();

				std::vector<app::Token> tokens;
				std::vector<model::Dog> dogs;
				for (auto p : tokens_.GetTokens()) {
					tokens.push_back(p.first);
					dogs.push_back(*(p.second->GetDog()));
				}

				auto lost_object = (*game_.GetGameSessions().begin())->GetCurrentLostObjects();

				GameState game_state = { map_id, dogs, tokens, lost_object };
				std::stringstream ss_;
				boost::archive::text_oarchive oa{ ss_ };
				oa << game_state;

				std::ofstream file;
				std::filesystem::path temp_path = state_file_path_.string().substr(0, state_file_path_.string().size() - 4) + "temp.txt";
				file.open(temp_path);
				file << ss_.str();
				file.close();
				std::filesystem::rename(temp_path, state_file_path_);
			
			}
		}

		void Deserialize() {
			GameState game_state;
			std::stringstream ss_;
			std::ifstream file;
			file.open(state_file_path_);
			ss_ << file.rdbuf();
			boost::archive::text_iarchive ia { ss_ };
			ia >> game_state;
			file.close();
			auto gs = game_.FindGameSession(model::Map::Id(game_state.map_id));
			for (int i = 0; i < game_state.dogs.size(); ++i) {
				tokens_.AddPlayer(game_state.tokens[i], players_.Add(std::make_shared<model::Dog>(game_state.dogs[i]), gs, "Regain"));
			}
			gs->GetCurrentLostObjects() = game_state.lost_objects;

		}

		void SetSaveInterval(double save_interval) {
			save_interval_ = save_interval;
		}
		void SetParams(double is_save, double is_auto_save, double save_interval, std::filesystem::path state_file_path) {
			is_save_ = is_save;
			is_auto_save_ = is_auto_save;
			save_interval_ = save_interval;
			state_file_path_ = state_file_path;
		}

	private:
		double save_interval_ = 5000.;
		double time_since_save_ = 0.;

		bool is_save_ = false;
		bool is_auto_save_ = false;



		std::filesystem::path state_file_path_;

	};
}//namespace app_serialization


//namespace boost::serialization {
//	template <typename Archive>
//	void serialize(Archive& ar, std::shared_ptr<app::Player>& object, const unsigned int file_version) {
//		ar& app_serialization::SerPlayer(*object);
//	}
//}
