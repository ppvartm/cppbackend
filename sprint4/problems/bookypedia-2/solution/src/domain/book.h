#pragma once
#include <optional>
#include <string>
#include <vector>

#include "../util/tagged_uuid.h"
#include "author.h"

namespace domain {

namespace detail {
struct BookTag {};
} // namespace detail

using BookId = util::TaggedUUID<detail::BookTag>;

class Book {
public:
  Book(BookId id, AuthorId author_id, std::string title, int year);
  Book(BookId id, AuthorId author_id, std::string title, int year,
       const std::string &author_name);

  const BookId &GetId() const noexcept;
  const AuthorId &GetAuthorId() const noexcept;
  const std::string &GetTitle() const noexcept;
  int GetPubYear() const noexcept;

  const std::vector<std::string> &GetTags() const noexcept;
  void SetTags(const std::vector<std::string> &tags);

  std::optional<std::string> GetAuthorName() const noexcept;

private:
  BookId id_;
  AuthorId author_id_;
  std::string title_;
  int year_;
  std::optional<std::string> author_name_;
  std::vector<std::string> tags_;
};

class BookRepository {
public:
  virtual void Save(const Book &book) = 0;
  virtual std::vector<Book> GetAll() const = 0;
  virtual std::vector<Book> GetAllByAuthor(const AuthorId &author_id) const = 0;
  virtual std::optional<Book> LoadById(const BookId &book_id) const = 0;
  virtual std::vector<domain::Book>
  LoadByTitle(const std::string &title) const = 0;
  virtual void Delete(const BookId &id) = 0;
  virtual void DeleteBooksByAuthorId(const AuthorId &author_id) = 0;
  virtual void Edit(const BookId &id, const std::string &title,
                    int pub_year) = 0;

protected:
  ~BookRepository() = default;
};

} // namespace domain