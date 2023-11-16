//#include "sdk.h"

#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "http_server.h"

#include "log.h"
#include "app.h"




using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;


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

}  // namespace






int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json>"sv << std::endl;
        return EXIT_FAILURE;
    }
    try {

       /* app::Players p;
        model::Map::Id id{ static_cast<std::string>("2") };
        p.FindByDogAndMapId(1, id);*/


        logging::add_common_attributes();
        logging::add_console_log(
            std::clog,
            logging::keywords::format = MyFormatter,
            logging::keywords::auto_flush = true
        );

        std::filesystem::path path1{ argv[1] };
       path1 = std::filesystem::weakly_canonical(path1);
        std::filesystem::path path2{ argv[2] };
        path2 = std::filesystem::weakly_canonical(path2);
        // 1. Загружаем карту из файла и построить модель игры
         model::Game game = json_loader::LoadGame(path1);
      //   model::Game game = json_loader::LoadGame("../data/config.json");
        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
            });

        
        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        http_handler::RequestHandler handler {game};
        handler.SetFilePath(path2);
       // handler.SetFilePath("../static");
        http_handler::LoggingRequestHandler logging_handler(handler);

       

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
  /*      http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });*/

        http_server::ServeHttp(ioc, { address, port }, [&logging_handler](auto&& req, auto&& send) {
            logging_handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
            });
        

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
       // std::cout << "Server has started..."sv << std::endl;
        ServerStartLog(port, address);
        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
        ServerStopLog(0);
        std::cin.get();
    } catch (const std::exception& ex) {
        ServerStopLog(EXIT_FAILURE, ex.what());
      //  std::cout << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::cin.get();
}