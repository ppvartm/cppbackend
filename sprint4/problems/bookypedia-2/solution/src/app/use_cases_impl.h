#pragma once

#include <vector>

#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "../domain/book_tag_fwd.h"
#include "unit_of_work.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
  explicit UseCasesImpl(UnitOfWorkFactory &unit_of_work_factory);
  // author
  void AddAuthor(const std::string &name) override;
  std::vector<domain::Author> ShowAuthors() const override;
  std::optional<domain::Author>
  ShowAuthorByName(const std::string &name) const override;
  std::optional<domain::Author>
  ShowAuthorById(const std::string &id) const override;
  bool DeleteAuthor(const std::string &id) override;
  bool EditAuthor(const std::string &id, const std::string &new_name) override;
  // book
  void AddBook(const std::string &author_id, const std::string &title,
               int pub_year, const std::vector<std::string> &tags) override;
  void AddBookWithAuthorName(const std::string &author_name,
                             const std::string &title, int pub_year,
                             const std::vector<std::string> &tags) override;
  std::vector<domain::Book> ShowBooks() const override;
  std::vector<domain::Book>
  ShowBooksByAuthorId(const std::string &author_id) const override;
  std::vector<domain::Book>
  ShowBooksByTitle(const std::string &title) const override;
  std::optional<domain::Book>
  ShowBookById(const std::string &id) const override;
  bool DeleteBook(const std::string &id) override;
  bool EditBook(const std::string &id, const std::string &title, int pub_year,
                const std::vector<std::string> &tags) override;

private:
  UnitOfWorkFactory &unit_of_work_factory_;
};

} // namespace app
