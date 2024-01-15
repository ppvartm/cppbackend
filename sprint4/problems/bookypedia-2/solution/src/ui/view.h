#pragma once
#include <iosfwd>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace menu {
class Menu;
}

namespace app {
class UseCases;
}

namespace ui {
namespace detail {

struct AddBookParamsWithAuthorId {
  std::string title;
  std::string author_id;
  std::vector<std::string> tags;
  int publication_year = 0;
};

struct AddBookParamsWithAuthorName {
  std::string title;
  std::string author_name;
  std::vector<std::string> tags;
  int publication_year = 0;
};

using AddBookParams =
    std::variant<AddBookParamsWithAuthorId, AddBookParamsWithAuthorName>;

struct EditBookParams {
  std::optional<std::string> title;
  std::vector<std::string> tags;
  std::optional<int> publication_year;
};

struct AuthorInfo {
  std::string id;
  std::string name;
};

struct BookInfoByAuthor {
  std::string title;
  int publication_year;
};

struct BookInfoWithAuthor {
  std::string id;
  std::string title;
  std::string author_name;
  int publication_year;
};

struct BookInfoCompletely {
  std::string id;
  std::string title;
  std::string author_name;
  int publication_year;
  std::vector<std::string> tags;
};

std::string GetTagsPrintable(const std::vector<std::string> &tags);

} // namespace detail

class View {
public:
  View(menu::Menu &menu, app::UseCases &use_cases, std::istream &input,
       std::ostream &output);

private:
  bool AddAuthor(std::istream &cmd_input) const;
  bool AddBook(std::istream &cmd_input) const;
  bool ShowAuthors() const;
  bool ShowBooks() const;
  bool ShowAuthorBooks() const;
  bool DeleteAuthor() const;
  bool DeleteAuthorWithName(std::istream &cmd_input) const;
  bool EditAuthor() const;
  bool EditAuthorWithName(std::istream &cmd_input) const;
  bool ShowBook() const;
  bool ShowBookWithTitle(std::istream &cmd_input) const;
  bool DeleteBook() const;
  bool DeleteBookWithTitle(std::istream &cmd_input) const;
  bool EditBook() const;
  bool EditBookWithTitle(std::istream &cmd_input) const;

  std::optional<detail::AddBookParams>
  GetBookParams(std::istream &cmd_input) const;
  detail::EditBookParams
  GetBookParamsForEdit(const detail::BookInfoCompletely &book) const;
  std::optional<std::string>
  EnterAuthorName(const std::string &introductory_phrase) const;
  bool OfferToAddAuthor(const std::string &author_name) const;
  std::optional<std::string> SelectAuthor() const;
  std::vector<std::string>
  EnterBookTags(const std::string &introductory_phrase) const;
  std::vector<detail::AuthorInfo> GetAuthors() const;
  std::vector<detail::BookInfoWithAuthor> GetBooks() const;
  std::vector<detail::BookInfoByAuthor>
  GetAuthorBooks(const std::string &author_id) const;
  std::optional<detail::AuthorInfo>
  GetAuthorByName(const std::string &name) const;
  std::optional<std::string>
  SelectBook(const std::vector<detail::BookInfoWithAuthor> &books) const;
  std::vector<detail::BookInfoWithAuthor>
  GetBooksByTitle(const std::string &title) const;
  std::optional<detail::BookInfoCompletely>
  GetBookById(const std::string &book_id) const;
  std::optional<std::string>
  EnterTitle(const std::string &introductory_phrase) const;
  std::optional<int> EnterPubYear(const std::string &introductory_phrase) const;

  menu::Menu &menu_;
  app::UseCases &use_cases_;
  std::istream &input_;
  std::ostream &output_;
};

} // namespace ui