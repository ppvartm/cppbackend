#include "postgres.h"
#include <pqxx/zview.hxx>
#include <pqxx/pqxx>
namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    pqxx::work work{connection_};
    work.exec_params("INSERT INTO authors (id, name) VALUES ($1, $2) ON CONFLICT (id) DO UPDATE SET name=$2;"_zv,
        author.GetId().ToString(), author.GetName());
    work.commit();
}

void BookRepositoryImpl::Save(const domain::Book& book) {
     pqxx::work work{ connection_ };
     work.exec_params("INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4) ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;"_zv,
            book.GetId().ToString(), book.GetAuthorIdStr(), book.GetTitle(), book.GetPublicationYear());
     work.commit();
   
}

void BookRepositoryImpl::Save(domain::BookId book_id, int author_id, const std::string& title, uint16_t publication_year) {
    std::string result;
   
     pqxx::work work{ connection_ };
    work.exec_params("INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4) ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;"_zv,
     book_id.ToString(), result, title, publication_year);
      work.commit();
   
    
}


std::string AuthorRepositoryImpl::GetAuthorId(int i) {
    std::string result;
    pqxx::read_transaction read(connection_);
    auto query_text = "SELECT name, id FROM authors ORDER BY name"_zv;
    int k = 1;
    for (auto [name, id] : read.query<std::string, std::string>(query_text)) {
        if (k++ == i) {
            result = id;
        }

    }
    return result;
}

std::vector<std::string> AuthorRepositoryImpl::GetListOfAuthors() {
    std::vector<std::string> result;
    pqxx::read_transaction read(connection_);
    auto query_text = "SELECT name FROM authors ORDER BY name"_zv;
    for (auto [name] : read.query<std::string>(query_text)) {
        result.push_back(name);
    }
    return result;

}

std::vector<std::pair<std::string, std::string>> AuthorRepositoryImpl::GetFullInfoAboutAuthors() {
    std::vector<std::pair<std::string, std::string>> result;
    pqxx::read_transaction read(connection_);
    auto query_text = "SELECT name, id FROM authors ORDER BY name"_zv;
    for (auto [name, id] : read.query<std::string, std::string>(query_text)) {
        result.push_back({name, id});
    }
    return result;

}


std::vector<std::pair<std::string, uint16_t>> BookRepositoryImpl::GetAllBooks() {
    std::vector<std::pair<std::string, uint16_t>> result;
    pqxx::read_transaction read(connection_);
    auto query_text = "SELECT title, publication_year FROM books ORDER BY title"_zv;
    for (auto [title, year] : read.query<std::string, uint16_t>(query_text)) {
        result.push_back({ title, year });
    }
    return result;
}

std::vector<std::pair<std::string, uint16_t>> BookRepositoryImpl::GetAuthorBooks(const std::string& author_id) {
    std::vector<std::pair<std::string, uint16_t>> result;
    pqxx::read_transaction read(connection_);
    auto str = "SELECT title, publication_year FROM books WHERE author_id = \'" + author_id + "\' ORDER BY publication_year, title";
    auto query_text = pqxx::zview(str.c_str());
    for (auto [title, year] : read.query<std::string, uint16_t>(query_text)) {
        result.push_back({ title, year });
    }
    return result;
}

std::vector<std::pair<std::string, uint16_t>> BookRepositoryImpl::GetAuthorBooks(int i) {
    std::string author_id;
    pqxx::read_transaction read(connection_);
    auto query_text = "SELECT name, id FROM authors ORDER BY name"_zv;
    int k = 1;
    for (auto [name, id] : read.query<std::string, std::string>(query_text)) {
        if (k++ == i) {
            author_id = id;
        }
    }
 
    std::vector<std::pair<std::string, uint16_t>> result;
    auto str = "SELECT title, publication_year FROM books WHERE author_id = \'" + author_id + "\' ORDER BY publication_year, title";
    query_text = pqxx::zview(str.c_str());
    for (auto [title, year] : read.query<std::string, uint16_t>(query_text)) {
        result.push_back({ title, year });
    }
    return result;
}



Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {

    pqxx::work work{connection_};

    work.exec("CREATE TABLE IF NOT EXISTS authors (id UUID CONSTRAINT author_id_constraint PRIMARY KEY, name varchar(100) UNIQUE NOT NULL);"_zv);

    work.exec("CREATE TABLE IF NOT EXISTS books( id UUID CONSTRAINT books_id_constraint PRIMARY KEY, author_id UUID NOT NULL, title varchar(100) NOT NULL, publication_year integer);"_zv);

    // коммитим изменения
    work.commit();
}

}  // namespace postgres