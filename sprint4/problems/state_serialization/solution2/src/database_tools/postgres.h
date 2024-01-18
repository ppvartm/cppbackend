#pragma once
#include <pqxx/pqxx>
#include <string>

#include <boost/json.hpp>

#include "../model/model.h"

namespace postgres_tools {

	using pqxx::operator"" _zv;


	struct Record {
		std::string name;
		int score;
		double play_time;
	};

	class PostgresDatabase {
	public:
		PostgresDatabase(const std::string& conn) : conn_{ conn } {
			pqxx::work w(conn_);
			w.exec(
				"CREATE TABLE IF NOT EXISTS retired_players (id SERIAL PRIMARY KEY, name varchar(100), score integer, time real);"_zv);
			w.commit();
		}

		void AddRecord(const std::string& name, int score, double play_time) {
			pqxx::work w(conn_);
			w.exec("INSERT INTO retired_players (name, score, time) VALUES (" + 
				w.quote(static_cast<std::string>(name)) + ", " + std::to_string(score) + ", " + std::to_string(play_time) + ")");
			w.commit();
		}

		std::vector<Record> GetRecords() {
			pqxx::read_transaction r(conn_);
			auto query_text = "SELECT name, score, time FROM retired_players ORDER BY score DESC, time ASC, name ASC"_zv;
			std::vector<Record> result;
			for (auto [name, score, time] : r.query<std::string, int, double>(query_text)) {
				result.push_back({ name, score, time });
			}
			return result;
		}

		void AddRecordsAllDogs(model::GameSession& game_session, std::vector<model::Dog::Id> list_id_for_deletion) {
			auto dogs = game_session.GetDogs();
			for (auto& p : dogs)
				for (auto id : list_id_for_deletion)
					if (p.second->GetId() == id)
						AddRecord(p.second->GetName(), p.second->GetScore(), p.second->GetPlayTime());
		}


	private:
		pqxx::connection conn_;
		
	};


	void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Record& record);

}//namespace postgres_tools