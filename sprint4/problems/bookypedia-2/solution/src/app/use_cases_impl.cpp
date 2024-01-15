#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"
#include "../domain/book_tag.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <optional>
#include <utility>
#include <vector>

namespace app {
using namespace domain;

UseCasesImpl::UseCasesImpl(UnitOfWorkFactory &unit_of_work_factory)
    : unit_of_work_factory_{unit_of_work_factory} {}

void UseCasesImpl::AddAuthor(const std::string &name) {
  auto transaction = unit_of_work_factory_.CreateUnitOfWork();
  transaction->Authors().Save({AuthorId::New(), name});
  transaction->Commit();
}

std::vector<domain::Author> UseCasesImpl::ShowAuthors() const {
  auto transaction = unit_of_work_factory_.CreateUnitOfWork();
  auto res = transaction->Authors().GetAll();
  transaction->Commit();
  return std::move(res);
}

std::optional<domain::Author>
UseCasesImpl::ShowAuthorByName(const std::string &name) const {
  auto transaction = unit_of_work_factory_.CreateUnitOfWork();
  auto res = transaction->Authors().LoadByName(name);
  transaction->Commit();
  return std::move(res);
}

std::optional<domain::Author>
UseCasesImpl::ShowAuthorById(const std::string &id) const {
  auto transaction = unit_of_work_factory_.CreateUnitOfWork();
  auto res = transaction->Authors().LoadById(AuthorId::FromString(id));
  transaction->Commit();
  return std::move(res);
}

bool UseCasesImpl::DeleteAuthor(const std::string &id) {
  auto transaction = unit_of_work_factory_.CreateUnitOfWork();
  if (const auto author =
          transaction->Authors().LoadById(AuthorId::FromString(id))) {
    for (const auto &book :
         transaction->Books().GetAllByAuthor(author->GetId())) {
      transaction->BookTags().DeleteByBookId(book.GetId());
    }
    transaction->Books().DeleteBooksByAuthorId(author->GetId());
    transaction->Authors().Delete(author->GetId());
    transaction->Commit();
    return true;
  }
  transaction->Commit();
  return false;
}

bool UseCasesImpl::EditAuthor(const std::string &id,
                              const std::string &new_name) {
  auto transaction = unit_of_work_factory_.CreateUnitOfWork();
  if (const auto author =
          transaction->Authors().LoadById(domain::AuthorId::FromString(id))) {
    transaction->Authors().Edit(author->GetId(), new_name);
    transaction->Commit();
    return true;
  }
  transaction->Commit();
  return false;
}

void UseCasesImpl::AddBook(const std::string &author_id,
                           const std::string &title, int pub_year,
                           const std::vector<std::string> &tags) {
  auto transaction = unit_of_work_factory_.CreateUnitOfWork();
  auto book_id = BookId::New();
  transaction->Books().Save(
      {book_id, AuthorId::FromString(author_id), title, pub_year});
  for (const std::string &tag : tags) {
    transaction->BookTags().Save(BookTag{book_id, tag});
  }
  transaction->Commit();
}

std::vector<domain::Book> UseCasesImpl::ShowBooks() const {
  auto transaction = unit_of_work_factory_.CreateUnitOfWork();
  auto res = transaction->Books().GetAll();
  transaction->Commit();
  return std::move(res);
}

std::vector<domain::Book>
UseCasesImpl::ShowBooksByAuthorId(const std::string &author_id) const {
  auto transaction = unit_of_work_factory_.CreateUnitOfWork();
  auto res =
      transaction->Books().GetAllByAuthor(AuthorId::FromString(author_id));
  transaction->Commit();
  return std::move(res);
}

void UseCasesImpl::AddBookWithAuthorName(const std::string &author_name,
                                         const std::string &title, int pub_year,
                                         const std::vector<std::string> &tags) {
  auto transaction = unit_of_work_factory_.CreateUnitOfWork();
  auto author_id = AuthorId::New();
  transaction->Authors().Save({author_id, author_name});
  auto book_id = BookId::New();
  transaction->Books().Save({book_id, author_id, title, pub_year});
  for (const std::string &tag : tags) {
    transaction->BookTags().Save(BookTag{book_id, tag});
  }
  transaction->Commit();
}

// std::optional<domain::Book>
// UseCasesImpl::ShowBookById(const std::string &id) const {
//   auto transaction = unit_of_work_factory_.CreateUnitOfWork();
//   auto res = transaction->Books().LoadById(BookId::FromString(id));
//   transaction->Commit();
//   return std::move(res);
// }

std::vector<domain::Book>
UseCasesImpl::ShowBooksByTitle(const std::string &title) const {
  auto transaction = unit_of_work_factory_.CreateUnitOfWork();
  auto res = transaction->Books().LoadByTitle(title);
  transaction->Commit();
  return std::move(res);
}

std::optional<domain::Book>
UseCasesImpl::ShowBookById(const std::string &id) const {
  auto transaction = unit_of_work_factory_.CreateUnitOfWork();
  BookId book_id = BookId::FromString(id);
  if (auto book = transaction->Books().LoadById(book_id)) {
    auto book_tags = transaction->BookTags().GetAllByBookId(book_id);
    std::vector<std::string> tags;
    tags.reserve(book_tags.size());
    std::transform(book_tags.begin(), book_tags.end(),
                   std::inserter(tags, tags.end()),
                   [](const BookTag &book_tag) { return book_tag.GetTag(); });
    book->SetTags(tags);
    transaction->Commit();
    return book;
  }
  transaction->Commit();
  return std::nullopt;
}

bool UseCasesImpl::DeleteBook(const std::string &id) {
  auto transaction = unit_of_work_factory_.CreateUnitOfWork();
  if (auto book = transaction->Books().LoadById(BookId::FromString(id))) {
    transaction->BookTags().DeleteByBookId(book->GetId());
    transaction->Books().Delete(book->GetId());
    transaction->Commit();
    return true;
  }
  transaction->Commit();
  return false;
}

bool UseCasesImpl::EditBook(const std::string &id, const std::string &title,
                            int pub_year,
                            const std::vector<std::string> &tags) {
  auto transaction = unit_of_work_factory_.CreateUnitOfWork();
  if (auto book = transaction->Books().LoadById(BookId::FromString(id))) {
    transaction->Books().Edit(book->GetId(), title, pub_year);
    auto book_tags = transaction->BookTags().GetAllByBookId(book->GetId());
    if (!book_tags.empty()) {
      transaction->BookTags().DeleteByBookId(book->GetId());
    }
    for (const auto &tag : tags) {
      transaction->BookTags().Save(BookTag{book->GetId(), tag});
    }
    transaction->Commit();
    return true;
  }
  transaction->Commit();
  return false;
}

} // namespace app
