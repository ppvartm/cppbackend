#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

void UseCasesImpl::AddBook(const std::string& author_id, const std::string& title, uint16_t publication_year) {
    books_.Save({ BookId::New(), author_id, title, publication_year });
}


std::vector<std::pair<std::string, uint16_t>> UseCasesImpl::GetAllBooks() {
    return books_.GetAllBooks();
}

std::vector<std::string> UseCasesImpl::GetAllAuthors() {
    return authors_.GetListOfAuthors();
}

std::vector<std::pair<std::string, std::string>> UseCasesImpl::GetFullInfoAboutAuthors() {
    return authors_.GetFullInfoAboutAuthors();
}


std::vector<std::pair<std::string, uint16_t>> UseCasesImpl::GetAuthorBooks(const std::string& author_id) {
    return books_.GetAuthorBooks(author_id);
}




}  // namespace app
