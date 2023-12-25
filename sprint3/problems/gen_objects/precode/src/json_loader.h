#pragma once

#include <filesystem>
#include <fstream>

#include <boost/json.hpp>

#include "model.h"

namespace json_loader {
	namespace json = boost::json;

	model::Map MakeMap(const json::object& json_map);

	void AddRoads(const json::object& json_map, model::Map& map);
	void AddBuildings(const json::object& json_map, model::Map& map);
	void AddOffices(const json::object& json_map, model::Map& map);

	void SetDogSpeed(const json::object& json_map, model::Map& map);

	model::Game LoadGame(std::filesystem::path json_path);
 
}   // namespace json_loader
