//#include "sdk.h"
#include <boost/program_options.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "http_server.h"

#include "log.h"
#include "app.h"
#include "timer.h"



using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace po = boost::program_options;

namespace {

 // Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned num_of_threads, const Fn& fn) {
    num_of_threads = std::max(1u, num_of_threads);
    std::vector<std::jthread> workers;
    workers.reserve(num_of_threads - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--num_of_threads) {
        workers.emplace_back(fn);
    }
    fn();

}

// структура параметров командной строки 
struct Args {
    std::string tick_period;
    std::string config_file_path;
    std::string static_dir_path;
    std::string random_spawn;
};

std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    po::options_description desc{ "All options"s };
    Args args;
    desc.add_options()
        ("help,h", "Show help")
        ("tick-period,t", po::value(&args.tick_period)->value_name("millisecondse"s), "Set tick period")
        ("config-file,c", po::value(&args.config_file_path)->value_name("file"s), "Set config file path")
        ("www-root,w", po::value(&args.static_dir_path)->value_name("dir"s), "Set static files root")
        ("randomize-spawn-points", "Set static files root");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        std::cout << desc;
        return std::nullopt;
    }

    // Проверяем наличие опций src и dst
    if (!vm.contains("tick-period"s)) {

    }
    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("Config file path is not specified"s);
    }
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("Static dir path is not specified"s);
    }
    if (!vm.contains("randomize-spawn-points"s)) {
       // args.random_spawn = "";
    }
    if (!vm.contains("tick-period"s)) {

    }
    return args;
  }
}  // namespace


int main(int argc, const char* argv[]) {
    //if (argc != 3) {
    //    std::cerr << "Usage: game_server <game-config-json>"sv << std::endl;
    //    return EXIT_FAILURE;
    //}
    try {
        logging::add_common_attributes();
        logging::add_console_log(
            std::clog,
            logging::keywords::format = MyFormatter,
            logging::keywords::auto_flush = true
        );
        auto args = ParseCommandLine(argc, argv);
        //std::cout << args->config_file_path << "\n";
        //std::cout << args->static_dir_path << "\n";
        //std::cout << args->random_spawn << "\n";
        //std::cout << args->tick_period << "\n";


        
        std::filesystem::path path1{ args->config_file_path };
        std::filesystem::path path2{ args->static_dir_path };
        if (!std::filesystem::exists(path1))
            throw std::runtime_error("Config file doesn't exist");
        if (!std::filesystem::exists(path2))
            throw std::runtime_error("Static files dir doesn't exist");

       // std::filesystem::path path1{ argv[1] };
        path1 = std::filesystem::weakly_canonical(path1);
       // std::filesystem::path path2{ argv[2] };
        path2 = std::filesystem::weakly_canonical(path2);
     //    1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(path1);
        if (args->random_spawn != "")
            game.SetRandomSpawn();

       // model::Game game = json_loader::LoadGame("../data/config.json");
      //   2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);
      //  2.1. Добавляем strand
        auto api_strand = net::make_strand(ioc);

       //  3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

      //   4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        http_handler::RequestHandler handler {game, api_strand};
        handler.SetFilePath(path2);
       // handler.SetFilePath("../static");
      //  http_handler::LoggingRequestHandler logging_handler(handler);
        
       

       //  5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, { address, port }, [&handler](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
            });
 

        // 5.1. Добавляем автоматическое обновление времени, если это нужно
         
        //std::shared_ptr<Timer::Ticker> ticker;
        //if (args->tick_period != "") {
        //    handler.SetAutomaticTick();
        //      ticker = std::make_shared<Timer::Ticker>(api_strand, std::chrono::milliseconds(std::stoi(args->tick_period)),
        //         [&handler](std::chrono::milliseconds delta) {
        //             handler.Tick(delta); 
        //         });
        //      ticker->Start();
        //}

      //   Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        std::cout << "Server has started..."sv << std::endl;
        ServerStartLog(port, address);
      //   6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
        ServerStopLog(0);
        std::cin.get();
    } catch (const std::exception& ex) {
        ServerStopLog(EXIT_FAILURE, ex.what());
        return EXIT_FAILURE;
    }
   // std::cin.get();
}
