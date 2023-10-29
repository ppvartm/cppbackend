#pragma once

#include <filesystem>
#include <fstream>
#include "model.h"
#include <boost/json.hpp>

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path);
 

}  // namespace json_loader
