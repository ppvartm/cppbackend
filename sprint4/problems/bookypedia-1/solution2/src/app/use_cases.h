#pragma once

#include <string>
#include "../domain/author.h"
#include "../domain/book.h"

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual void AddBook(const domain::AuthorId& author_id, const std::string& title, uint16_t publication_year) = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
