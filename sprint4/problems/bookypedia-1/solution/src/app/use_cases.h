#pragma once

#include <string>
#include <vector>
#include "../domain/author.h"
#include "../domain/book.h"

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual void AddBook(const std::string& author_id, const std::string& title, uint16_t publication_year) = 0;
    virtual std::vector<std::pair<std::string, uint16_t>> GetAllBooks() = 0;
    virtual std::vector<std::string> GetAllAuthors() = 0;
    virtual std::vector<std::pair<std::string, uint16_t>> GetAuthorBooks(const std::string& author_id) = 0;
    virtual std::vector<std::pair<std::string, std::string>> GetFullInfoAboutAuthors() = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
