#include <iostream>
#include <pqxx/pqxx>
#include <boost/json.hpp>

using namespace std::literals;

using pqxx::operator"" _zv;


int main(int argc, char** argv) {
	try {
		if (argc == 1) {
			std::cout << "Usage postgres_test <conn-string>\n"sv;
			return EXIT_SUCCESS;
		}
		else if (argc != 2) {
			std::cerr << "Invalid command line \n"sv;
			return EXIT_FAILURE;
		}
		pqxx::connection conn{ argv[1] };

		pqxx::work w(conn);

		w.exec(
			"CREATE TABLE IF NOT EXISTS books (id SERIAL PRIMARY KEY, title varchar(100) NOT NULL, author varchar(100) NOT NULL, year integer NOT NULL, ISBN char(13) UNIQUE);"_zv);

		w.commit();

		std::string json_request;

		while (true) {
			std::getline(std::cin, json_request);
			std::cout << json_request << "\n";
			auto value = boost::json::parse(json_request);
			auto action = value.as_object().at("action").as_string();
			if (action == "exit")
				break;
			if (action == "all_books") {
				pqxx::read_transaction r(conn);
				auto query_text = "SELECT id, title, author, year, ISBN FROM books ORDER BY year DESC, title ASC, author ASC, ISBN ASC"_zv;
				boost::json::array json_response;
				for (auto [id, title, author, year, ISBN] : r.query<int, std::string_view, std::string_view, int, std::optional<std::string>>(query_text)) {
					if (ISBN.has_value()) {
						boost::json::value book = {
							{"id", id},
							{"title", title},
							{"author", author},
							{"year", year},
							{"ISBN", ISBN.value()}
						};
						json_response.push_back(book);
					} 
					else{ 
					    boost::json::value book = {
								{"id", id},
								{"title", title},
								{"author", author},
								{"year", year},
								{"ISBN", nullptr}
							};
							json_response.push_back(book);
						}
				}
				std::cout << json_response << "\n";
				continue;
			}
			if (action == "add_book") {
				auto payload = value.as_object().at("payload").as_object();
				auto title = payload.at("title").as_string();
				auto author = payload.at("author").as_string();
				int64_t year = payload.at("year").as_int64();
				auto ISBN = payload.at("ISBN");
				try {
					pqxx::work ww(conn);
					if (!ISBN.is_null())
				    	ww.exec("INSERT INTO books (title, author, year, ISBN) VALUES (" + ww.quote(static_cast<std::string>(title)) + ", " + ww.quote(static_cast<std::string>(author)) + ", " + std::to_string(year) + ", " + ww.quote(static_cast<std::string>(ISBN.as_string())) + " )");
					else
						ww.exec("INSERT INTO books (title, author, year, ISBN) VALUES (" + ww.quote(static_cast<std::string>(title)) + ", " + ww.quote(static_cast<std::string>(author)) + ", " + std::to_string(year) + ", DEFAULT)");
					ww.commit();
					boost::json::value json_response = {
						{"result", true}
					};
					std::cout << json_response << "\n";
				}
				catch (const std::exception& e) {
					boost::json::value json_response = {
					    {"result", false}
					};
					std::cout << json_response<<"\n";
					std::cout << e.what() << "\n";
				}
			

			}

		}
		
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}