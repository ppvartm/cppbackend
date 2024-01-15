#include "postgres.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>
#include <pqxx/internal/result_iterator.hxx>
#include <pqxx/zview.hxx>
#include <string>
#include <vector>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

// AuthorRepositoryImpl
AuthorRepositoryImpl::AuthorRepositoryImpl(pqxx::work &work) : work_{work} {}

void AuthorRepositoryImpl::Save(const domain::Author &author) {
  work_.exec_params(
      R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
)"_zv,
      author.GetId().ToString(), author.GetName());
}

std::vector<domain::Author> AuthorRepositoryImpl::GetAll() const {
  auto result = work_.exec(
      R"(
SELECT * FROM authors ORDER BY name;
						)"_zv);
  return GetAuthorsFromResult(result);
}

std::optional<domain::Author>
AuthorRepositoryImpl::LoadByName(const std::string &name) const {
  try {
    auto row = work_.exec_params1(
        R"(
SELECT * FROM authors WHERE name=$1;
						)"_zv,
        name);
    return GetAuthorFromRow(row);
  } catch (pqxx::unexpected_rows &) {
    return std::nullopt;
  }
}

std::optional<domain::Author>
AuthorRepositoryImpl::LoadById(const domain::AuthorId &id) const {
  try {
    auto row = work_.exec_params1(
        R"(
SELECT * FROM authors WHERE id=$1;
						)"_zv,
        id.ToString());
    return GetAuthorFromRow(row);
  } catch (pqxx::unexpected_rows &err) {
    return std::nullopt;
  }
}

std::vector<domain::Author>
AuthorRepositoryImpl::GetAuthorsFromResult(const pqxx::result &result) const {
  std::vector<domain::Author> authors;
  authors.reserve(result.size());
  for (const auto &row : result) {
    authors.emplace_back(GetAuthorFromRow(row));
  }
  return authors;
}

domain::Author
AuthorRepositoryImpl::GetAuthorFromRow(const pqxx::row &row) const {
  return {domain::AuthorId::FromString(row.at("id"s).as<std::string>()),
          row.at("name"s).as<std::string>()};
}

void AuthorRepositoryImpl::Delete(const domain::AuthorId &id) {
  work_.exec_params(R"(
    DELETE FROM authors WHERE id=$1;
  )"_zv,
                    id.ToString());
}

void AuthorRepositoryImpl::Edit(const domain::AuthorId &id,
                                const std::string &name) {
  work_.exec_params(R"(
    UPDATE authors
    SET name=$2
    WHERE id=$1;
  )",
                    id.ToString(), name);
}

// BookRepositoryImpl
BookRepositoryImpl::BookRepositoryImpl(pqxx::work &work) : work_{work} {}

void BookRepositoryImpl::Save(const domain::Book &book) {
  work_.exec_params(
      R"(
INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;
)"_zv,
      book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(),
      book.GetPubYear());
}

std::vector<domain::Book> BookRepositoryImpl::GetAll() const {
  auto result = work_.exec(
      R"(
SELECT 
    books.id AS book_id,
    author_id,
    authors.name AS name,
    title,
    publication_year
FROM books
INNER JOIN authors ON authors.id = author_id
ORDER BY title, name, publication_year;
						)"_zv);
  return GetBooksFromResult(result, {true});
}

std::vector<domain::Book>
BookRepositoryImpl::GetAllByAuthor(const domain::AuthorId &author_id) const {
  auto result = work_.exec_params(
      R"(
SELECT
    id AS book_id,
    author_id,
    title,
    publication_year
FROM books
WHERE author_id=$1
ORDER BY publication_year, title;
						)"_zv,
      author_id.ToString());
  return GetBooksFromResult(result);
}

// SELECT
//     id AS book_id,
//     author_id,
//     title,
//     publication_year
// FROM books
// WHERE books.id=$1;

std::optional<domain::Book>
BookRepositoryImpl::LoadById(const domain::BookId &book_id) const {
  try {
    auto row = work_.exec_params1(
        R"(
SELECT 
    books.id AS book_id,
    author_id,
    authors.name AS name,
    title,
    publication_year
FROM books
INNER JOIN authors ON authors.id = author_id
WHERE books.id=$1;
						)"_zv,
        book_id.ToString());
    return GetBookFromRow(row, {true});
  } catch (pqxx::unexpected_rows &) {
    return std::nullopt;
  }
}

std::vector<domain::Book>
BookRepositoryImpl::LoadByTitle(const std::string &title) const {
  auto result = work_.exec_params(
      R"(
SELECT 
    id AS book_id,
    author_id,
    title,
    publication_year
FROM books
WHERE title=$1;
						)"_zv,
      title);
  return GetBooksFromResult(result);
}

void BookRepositoryImpl::Delete(const domain::BookId &id) {
  work_.exec_params(R"(
DELETE FROM books
WHERE id=$1;
  )"_zv,
                    id.ToString());
}

void BookRepositoryImpl::Edit(const domain::BookId &id,
                              const std::string &title, int pub_year) {
  work_.exec_params(R"(
UPDATE books
SET title=$2, publication_year=$3
WHERE id=$1;
  )",
                    id.ToString(), title, pub_year);
}

void BookRepositoryImpl::DeleteBooksByAuthorId(
    const domain::AuthorId &author_id) {
  work_.exec_params(R"(
DELETE FROM books
WHERE author_id=$1;
  )"_zv,
                    author_id.ToString());
}

std::vector<domain::Book>
BookRepositoryImpl::GetBooksFromResult(const pqxx::result &result,
                                       GetResultConfig config) const {
  std::vector<domain::Book> books;
  books.reserve(result.size());
  for (const auto &row : result) {
    books.emplace_back(GetBookFromRow(row, config));
  }
  return books;
}

domain::Book BookRepositoryImpl::GetBookFromRow(const pqxx::row &row,
                                                GetResultConfig config) const {
  std::string book_id = row.at("book_id"s).as<std::string>();
  std::string author_id = row.at("author_id"s).as<std::string>();
  std::string title = row.at("title"s).as<std::string>();
  int pub_year = row.at("publication_year"s).as<int>();

  if (config.with_author_name) {
    return {domain::BookId::FromString(book_id),
            domain::AuthorId::FromString(author_id), title, pub_year,
            row.at("name").as<std::string>()};
  } else {
    return {domain::BookId::FromString(book_id),
            domain::AuthorId::FromString(author_id), title, pub_year};
  }
}

// BookTagRepositoryImpl
BookTagRepositoryImpl::BookTagRepositoryImpl(pqxx::work &work) : work_{work} {}

void BookTagRepositoryImpl::Save(const domain::BookTag &book_tag) {
  work_.exec_params(
      R"(
INSERT INTO book_tags (book_id, tag) VALUES ($1, $2);
)"_zv,
      book_tag.GetBookId().ToString(), book_tag.GetTag());
}

std::vector<domain::BookTag>
BookTagRepositoryImpl::GetAllByBookId(const domain::BookId &book_id) const {
  auto result = work_.exec_params(
      R"(
SELECT * FROM book_tags
WHERE book_id=$1
ORDER BY tag;
						)"_zv,
      book_id.ToString());
  return GetBookTagsFromResult(result);
}

void BookTagRepositoryImpl::DeleteByBookId(
    const domain::BookId &book_id) const {
  work_.exec_params(R"(
    DELETE FROM book_tags
    WHERE book_id=$1;
  )"_zv,
                    book_id.ToString());
}

std::vector<domain::BookTag>
BookTagRepositoryImpl::GetBookTagsFromResult(const pqxx::result &result) const {
  std::vector<domain::BookTag> book_tags;
  book_tags.reserve(result.size());
  for (const auto &row : result) {
    book_tags.emplace_back(GetBookTagFromRow(row));
  }
  return book_tags;
}

domain::BookTag
BookTagRepositoryImpl::GetBookTagFromRow(const pqxx::row &row) const {
  return domain::BookTag{
      domain::BookId::FromString(row.at("book_id"s).as<std::string>()),
      row.at("tag"s).as<std::string>()};
}

// UnitOfWorkImpl
UnitOfWorkImpl::UnitOfWorkImpl(pqxx::connection &connection)
    : work_{connection} {}

void UnitOfWorkImpl::Commit() { work_.commit(); }

AuthorRepositoryImpl &UnitOfWorkImpl::Authors() { return authors_; }
BookRepositoryImpl &UnitOfWorkImpl::Books() { return books_; }
BookTagRepositoryImpl &UnitOfWorkImpl::BookTags() { return book_tags_; }

// UnitOfWorkFactoryImpl
UOWFactoryImpl::UOWFactoryImpl(pqxx::connection &connection)
    : connection_{connection} {}

std::unique_ptr<app::UnitOfWork> UOWFactoryImpl::CreateUnitOfWork() {
  return std::make_unique<UnitOfWorkImpl>(connection_);
}

// Database
Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
  pqxx::work work{connection_};
  work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
    name varchar(100) UNIQUE NOT NULL
);
)"_zv);
  work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
    author_id UUID REFERENCES authors (id),
    title varchar(100) NOT NULL,
    publication_year integer
);
)"_zv);
  work.exec(R"(
CREATE TABLE IF NOT EXISTS book_tags (
    book_id UUID REFERENCES books (id),
    tag varchar(30) NOT NULL
);
)"_zv);
  // коммитим изменения
  work.commit();
}

UOWFactoryImpl &Database::GetUOWFactory() { return uow_factory_; }

} // namespace postgres