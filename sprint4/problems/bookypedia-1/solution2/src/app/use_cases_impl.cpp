#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

void UseCasesImpl::AddBook(const domain::AuthorId& author_id, const std::string& title, uint16_t publication_year) {
    books_.Save({ BookId::New(), author_id, title, publication_year });
}

}  // namespace app
