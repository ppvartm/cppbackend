#include "request_handler.h"




namespace http_handler {
    

    json::value MapNotFound() {
        json::value val = {
                    {"code", "mapNotFound"},
                    {"message","Map not found"} };
        return val;
    }
    json::value BadRequest() {
        json::value val = {
                   {"code", "badRequest"},
                   {"message","Bad request"} };
        return val;
    }
    std::string GetMaps(const model::Game& game) {
        std::vector<MapInfo> maps_info;
        auto maps = game.GetMaps();
        for (int i = 0; i < maps.size(); ++i)
            maps_info.emplace_back(maps[i]);
        std::string answ = json::serialize(json::value_from(maps_info));
        return answ;
    }
    std::string GetFileType(std::string file_name) {
        std::string answ;
        auto it = file_name.rbegin();
        while (*it != '.' && it !=  file_name.rend()) {
            *it = std::tolower(*it);
            answ = *(it++) + answ;
        }
        if (answ == "htm" || answ == "html")
            return "text/html";
        if (answ == "css")
            return "text/css";
        if (answ == "txt")
            return "text/plain";
        if (answ == "js")
            return "text/javascript";
        if (answ == "json")
            return "application/json";
        if (answ == "xml")
            return "application/xml";
        if (answ == "png")
            return "image/png";
        if (answ == "jpg" || answ == "jpeg" || answ == "jpe")
            return "image/jpeg";
        if (answ == "gif")
            return "image/gif";
        if (answ == "bmp")
            return "image/bmp";
        if (answ == "ico")
            return "image/vnd.microsoft.icon";
        if (answ == "tiff" || answ == "tif")
            return "image/tiff";
        if (answ == "svg" || answ == "svgz")
            return "image/svg+xml";
        if (answ == "mp3")
            return "audio/mpeg";
        return "application/octet-stream";
    }
    bool IsFileExist(const std::filesystem::path& file_path) {
        return std::filesystem::exists(file_path);
    }
    http::file_body::value_type ReadFile(const std::string& file_path) {
        http::file_body::value_type file;
        boost::system::error_code ec;
        file.open(file_path.data(), beast::file_mode::read, ec);
        if (ec)
          throw("Failed to open static-file: " + ec.message());
        return file;
    }


    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, MapInfo const& map_info) {
        jv = {
            {"id", *map_info.id_},
            {"name", map_info.name_}
        };
    }

}   // namespace http_handler
