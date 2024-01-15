#pragma once

#include <optional>
#include <string>
#include <vector>

namespace domain {
class Author;
class Book;
class BookTag;
} // namespace domain

namespace app {

class UseCases {
public:
  // author
  virtual void AddAuthor(const std::string &name) = 0;
  virtual std::vector<domain::Author> ShowAuthors() const = 0;
  virtual std::optional<domain::Author>
  ShowAuthorByName(const std::string &name) const = 0;
  virtual std::optional<domain::Author>
  ShowAuthorById(const std::string &id) const = 0;
  virtual bool DeleteAuthor(const std::string &id) = 0;
  virtual bool EditAuthor(const std::string &id,
                          const std::string &new_name) = 0;
  // book
  virtual void AddBook(const std::string &author_id, const std::string &title,
                       int pub_year, const std::vector<std::string> &tags) = 0;
  virtual void AddBookWithAuthorName(const std::string &author_name,
                                     const std::string &title, int pub_year,
                                     const std::vector<std::string> &tags) = 0;
  virtual std::vector<domain::Book> ShowBooks() const = 0;
  virtual std::vector<domain::Book>
  ShowBooksByAuthorId(const std::string &author_id) const = 0;
  virtual std::optional<domain::Book>
  ShowBookById(const std::string &id) const = 0;
  virtual std::vector<domain::Book>
  ShowBooksByTitle(const std::string &title) const = 0;
  virtual bool DeleteBook(const std::string &id) = 0;
  virtual bool EditBook(const std::string &id, const std::string &title,
                        int pub_year, const std::vector<std::string> &tags) = 0;

protected:
  ~UseCases() = default;
};

} // namespace app
