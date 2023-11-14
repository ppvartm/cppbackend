#pragma once

#include <filesystem>
#include <fstream>
#include "model.h"
#include <boost/json.hpp>

namespace json_loader {
	namespace json = boost::json;

	model::Game LoadGame(std::filesystem::path json_path);
 

}   // namespace json_loader
