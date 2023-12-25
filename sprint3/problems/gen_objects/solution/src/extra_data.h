#pragma once

#include <boost/json.hpp>




namespace extra_data{

	class Json_data {
	public:

		void Push(const boost::json::array& data, const std::string& map_id) {
			lootTypes_for_all_Maps_[map_id] = data;
		}

		//void PushBack(const boost::json::array& data) {
		//	lootTypes_for_all_Maps_.push_back(std::move(data));
	    //	}

		//boost::json::array Get(int i) {
		//	return lootTypes_for_all_Maps_[i];
	    //	}

		const boost::json::array& Get(const std::string& map_id) const {
			return lootTypes_for_all_Maps_.at(map_id);
		}

	private:
		//std::vector<boost::json::array>	lootTypes_for_all_Maps_;
		std::map< std::string, boost::json::array> lootTypes_for_all_Maps_;
	};

}