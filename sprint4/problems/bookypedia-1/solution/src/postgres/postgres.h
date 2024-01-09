#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "../domain/author.h"
#include "../domain/book.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const domain::Author& author) override;

    std::vector<std::string> GetListOfAuthors() override;
    std::string GetAuthorId(int i) override;

private:
    pqxx::connection& connection_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::connection& connection)
        : connection_{ connection } {
    }

    void Save(const domain::Book& book) override;
     
    void Save(domain::BookId book_id, int author_id, const std::string& title, uint16_t publication_year) override;

    std::vector<std::pair<std::string, uint16_t>> GetAllBooks() override;
    std::vector<std::pair<std::string, uint16_t>> GetAuthorBooks(const std::string& author_id) override;
    std::vector<std::pair<std::string, uint16_t>> GetAuthorBooks(int id) override;
private:
    pqxx::connection& connection_;

};

class Database {
public:
    explicit Database(pqxx::connection connection);

    AuthorRepositoryImpl& GetAuthors() & {
        return authors_;
    }

    BookRepositoryImpl& GetBooks()& {
        return books_;
    }

private:
    pqxx::connection connection_;
    AuthorRepositoryImpl authors_{connection_};
    BookRepositoryImpl books_{ connection_ };
};

}  // namespace postgres