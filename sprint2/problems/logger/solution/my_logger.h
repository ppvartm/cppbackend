#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
    std::chrono::system_clock::time_point GetTime() const {
        if (manual_ts_) {
            return *manual_ts_;
        }

        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::gmtime(&t_c), "%F %T");
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const {
        auto t_c = std::chrono::system_clock::to_time_t(GetTime());
        std::ostringstream oss;
        oss<< std::put_time(std::gmtime(&t_c), "%Y") <<"_"
           << std::put_time(std::gmtime(&t_c), "%m") <<"_"
           << std::put_time(std::gmtime(&t_c), "%d");
        return "/var/log/sample_log_"+oss.str()+".log" ;
    }

    Logger() = default;
   

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }
   
    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(Ts&&... args) {
        mutex_.lock();;
        log_file_.open(GetFileTimeStamp(), std::ios::app);
        //if (!manual_ts_.has_value())
            log_file_ << GetTimeStamp() << " : ";
        //else {
        //    log_file_ << *manual_ts_ << " : ";
           ((log_file_ << " " << std::forward<Ts>(args)), ...);
            log_file_ << "\n"sv;
        //}
        log_file_.close();
        mutex_.unlock();
    }

    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть 
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts){
        mutex_.lock();
        manual_ts_ = ts;
        mutex_.unlock();
    }

private:
    std::mutex mutex_;
    std::ofstream log_file_;

    std::optional<std::chrono::system_clock::time_point> manual_ts_;
};
