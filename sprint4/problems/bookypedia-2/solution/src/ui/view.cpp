#include "view.h"

#include <algorithm>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

#include "../app/use_cases.h"
#include "../domain/author.h"
#include "../domain/book.h"
#include "../menu/menu.h"
#include "../util/overload_pattern.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {
namespace detail {

std::ostream &operator<<(std::ostream &out, const AuthorInfo &author) {
  out << author.name;
  return out;
}

std::ostream &operator<<(std::ostream &out, const BookInfoByAuthor &book) {
  out << book.title << ", " << book.publication_year;
  return out;
}

std::ostream &operator<<(std::ostream &out, const BookInfoWithAuthor &book) {
  out << book.title << " by " << book.author_name << ", "
      << book.publication_year;
  return out;
}

std::ostream &operator<<(std::ostream &out, const BookInfoCompletely &book) {
  out << "Title: " << book.title << std::endl;
  out << "Author: " << book.author_name << std::endl;
  out << "Publication year: " << book.publication_year;
  if (!book.tags.empty()) {
    out << "\nTags: ";
    out << GetTagsPrintable(book.tags);
  }
  return out;
}

std::string GetTagsPrintable(const std::vector<std::string> &tags) {
  std::ostringstream ss;
  for (std::size_t i = 0; i < tags.size(); ++i) {
    ss << tags[i] << (tags.size() == i + 1 ? "" : ", ");
  }
  return ss.str();
}

} // namespace detail

template <typename T>
void PrintVector(std::ostream &out, const std::vector<T> &vector) {
  int i = 1;
  for (auto &value : vector) {
    out << i++ << " " << value << std::endl;
  }
}

View::View(menu::Menu &menu, app::UseCases &use_cases, std::istream &input,
           std::ostream &output)
    : menu_{menu}, use_cases_{use_cases}, input_{input}, output_{output} {
  menu_.AddAction("AddAuthor"s, "<name>"s, "Adds author"s,
                  std::bind(&View::AddAuthor, this, ph::_1));
  menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s,
                  std::bind(&View::AddBook, this, ph::_1));
  menu_.AddAction("ShowAuthors"s, {}, "Show authors"s,
                  std::bind(&View::ShowAuthors, this));
  menu_.AddAction("ShowBooks"s, {}, "Show books"s,
                  std::bind(&View::ShowBooks, this));
  menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s,
                  std::bind(&View::ShowAuthorBooks, this));
  menu_.AddAction("DeleteAuthor"s, "<name>"s, "Delete author"s,
                  std::bind(&View::DeleteAuthorWithName, this, ph::_1));
  menu_.AddAction("EditAuthor"s, "<name>"s, "Edit author"s,
                  std::bind(&View::EditAuthorWithName, this, ph::_1));
  menu_.AddAction("ShowBook"s, "<title>"s, "Show book"s,
                  std::bind(&View::ShowBookWithTitle, this, ph::_1));
  menu_.AddAction("DeleteBook"s, "<title>"s, "Delete book"s,
                  std::bind(&View::DeleteBookWithTitle, this, ph::_1));
  menu_.AddAction("EditBook"s, "<title>"s, "Edit book"s,
                  std::bind(&View::EditBookWithTitle, this, ph::_1));
}

bool View::AddAuthor(std::istream &cmd_input) const {
  try {
    std::string name;
    std::getline(cmd_input, name);
    boost::algorithm::trim(name);

    if (name.empty()) {
      throw std::logic_error("Empty name is not allowed"s);
    }

    if (auto author = GetAuthorByName(name)) {
      throw std::runtime_error("This author has been added before"s);
    }

    use_cases_.AddAuthor(std::move(name));
  } catch (const std::exception &) {
    output_ << "Failed to add author"sv << std::endl;
  }
  return true;
}

bool View::AddBook(std::istream &cmd_input) const {
  try {
    if (auto params = GetBookParams(cmd_input)) {
      std::visit(util::overload{
                     [this](detail::AddBookParamsWithAuthorId &p) {
                       use_cases_.AddBook(p.author_id, p.title,
                                          p.publication_year, p.tags);
                     },
                     [this](detail::AddBookParamsWithAuthorName &p) {
                       use_cases_.AddBookWithAuthorName(
                           p.author_name, p.title, p.publication_year, p.tags);
                     },
                 },
                 *params);
    }
  } catch (const std::exception &) {
    output_ << "Failed to add book"sv << std::endl;
  }
  return true;
}

bool View::ShowAuthors() const {
  PrintVector(output_, GetAuthors());
  return true;
}

bool View::ShowBooks() const {
  PrintVector(output_, GetBooks());
  return true;
}

bool View::ShowAuthorBooks() const {
  // TODO: handle error
  try {
    if (auto author_id = SelectAuthor()) {
      PrintVector(output_, GetAuthorBooks(*author_id));
    }
  } catch (const std::exception &) {
    throw std::runtime_error("Failed to Show Books");
  }
  return true;
}

bool View::DeleteAuthor() const {
  try {
    if (auto author_id = SelectAuthor()) {
      if (!use_cases_.DeleteAuthor(*author_id)) {
        throw std::logic_error("This author doesn't exist in the database"s);
      }
    }
  } catch (const std::exception &err) {
    output_ << "Failed to delete author"s << std::endl;
  }
  return true;
}

bool View::DeleteAuthorWithName(std::istream &cmd_input) const {
  std::string name;
  std::getline(cmd_input, name);
  boost::algorithm::trim(name);

  if (name.empty()) {
    return DeleteAuthor();
  }

  try {
    if (auto author = GetAuthorByName(name)) {
      if (!use_cases_.DeleteAuthor(author->id)) {
        throw std::logic_error("This author doesn't exist in the database"s);
      }
    } else {
      throw std::logic_error("This author doesn't exist in the database"s);
    }

  } catch (const std::exception &err) {
    output_ << "Failed to delete author"s << std::endl;
  }
  return true;
}

bool View::EditAuthor() const {
  try {
    if (auto author_id = SelectAuthor()) {
      if (auto new_name = EnterAuthorName("Enter new name:"s)) {
        if (!use_cases_.EditAuthor(*author_id, *new_name)) {
          throw std::runtime_error(""s);
        }
      } else {
        throw std::logic_error("Empty name is not allowed"s);
      }
    }
  } catch (const std::exception &) {
    output_ << "Failed to edit author"sv << std::endl;
  }
  return true;
}

bool View::EditAuthorWithName(std::istream &cmd_input) const {
  std::string name;
  std::getline(cmd_input, name);
  boost::algorithm::trim(name);

  if (name.empty()) {
    return EditAuthor();
  }

  try {
    if (auto author = use_cases_.ShowAuthorByName(name)) {
      if (auto new_name = EnterAuthorName("Enter new name:")) {
        if (!use_cases_.EditAuthor(author->GetId().ToString(), *new_name)) {
          throw std::runtime_error(""s);
        }
      } else {
        throw std::logic_error("Empty name is not allowed"s);
      }
    } else {
      throw std::logic_error("This author doesn't exist in the database"s);
    }

  } catch (const std::exception &) {
    output_ << "Failed to edit author"sv << std::endl;
  }
  return true;
}

bool View::ShowBook() const {
  try {
    if (const auto book_id = SelectBook(GetBooks())) {
      output_ << *GetBookById(*book_id) << std::endl;
    }
  } catch (const std::exception &) {
    throw std::runtime_error("Failed to Show Book");
  }
  return true;
}

bool View::ShowBookWithTitle(std::istream &cmd_input) const {
  std::string title;
  std::getline(cmd_input, title);
  boost::algorithm::trim(title);

  if (title.empty()) {
    return ShowBook();
  }

  try {
    const auto books = GetBooksByTitle(title);
    if (books.size() == 1) {
      output_ << *GetBookById(books.front().id) << std::endl;
    } else if (books.size() > 1) {
      if (const auto book_id = SelectBook(books)) {
        output_ << *GetBookById(*book_id) << std::endl;
      }
    }

  } catch (const std::exception &) {
    throw std::runtime_error("Failed to Show Book");
  }
  return true;
}

bool View::DeleteBook() const {
  try {
    if (const auto book_id = SelectBook(GetBooks())) {
      if (!use_cases_.DeleteBook(*book_id)) {
        throw std::logic_error("This book doesn't exist in the database"s);
      }
    }
  } catch (const std::exception &err) {
    output_ << "Book not found"s << std::endl;
  }
  return true;
}

bool View::DeleteBookWithTitle(std::istream &cmd_input) const {
  std::string title;
  std::getline(cmd_input, title);
  boost::algorithm::trim(title);

  if (title.empty()) {
    return DeleteBook();
  }

  try {
    const auto books = GetBooksByTitle(title);
    if (books.empty()) {
      throw std::logic_error("This book doesn't exist in the database"s);
    } else if (books.size() == 1) {
      if (!use_cases_.DeleteBook(books.front().id)) {
        throw std::logic_error("This book doesn't exist in the database"s);
      }
    } else {
      if (const auto book_id = SelectBook(books)) {
        if (!use_cases_.DeleteBook(*book_id)) {
          throw std::logic_error("This book doesn't exist in the database"s);
        }
      }
    }

  } catch (const std::exception &err) {
    output_ << "Book not found"s << std::endl;
  }
  return true;
}

bool View::EditBook() const {
  try {
    if (const auto book_id = SelectBook(GetBooks())) {
      if (const auto book = GetBookById(*book_id)) {
        auto params = GetBookParamsForEdit(*book);

        if (!params.title.has_value()) {
          params.title = book->title;
        }

        if (!params.publication_year.has_value()) {
          params.publication_year = book->publication_year;
        }

        if (!use_cases_.EditBook(book->id, *params.title,
                                 *params.publication_year, params.tags)) {
          throw std::runtime_error(""s);
        }
      }
    } else {
      throw std::runtime_error(""s);
    }
  } catch (const std::exception &) {
    output_ << "Book not found"sv << std::endl;
  }
  return true;
}

bool View::EditBookWithTitle(std::istream &cmd_input) const {
  std::string title;
  std::getline(cmd_input, title);
  boost::algorithm::trim(title);

  if (title.empty()) {
    return EditBook();
  }

  try {
    const auto books = GetBooksByTitle(title);
    detail::BookInfoCompletely edit_book;
    if (books.empty()) {
      throw std::logic_error("This book doesn't exist in the database"s);
    } else if (books.size() == 1) {
      edit_book = *GetBookById(books.front().id);
    } else {
      if (const auto book_id = SelectBook(books)) {
        edit_book = *GetBookById(*book_id);
      } else {
        throw std::runtime_error(""s);
      }
    }

    auto params = GetBookParamsForEdit(edit_book);

    if (!params.title.has_value()) {
      params.title = edit_book.title;
    }

    if (!params.publication_year.has_value()) {
      params.publication_year = edit_book.publication_year;
    }

    if (!use_cases_.EditBook(edit_book.id, *params.title,
                             *params.publication_year, params.tags)) {
      throw std::runtime_error(""s);
    }

  } catch (const std::exception &) {
    output_ << "Book not found"sv << std::endl;
  }
  return true;
}

std::optional<detail::AddBookParams>
View::GetBookParams(std::istream &cmd_input) const {
  int pub_year = 0;
  cmd_input >> pub_year;

  std::string title;
  std::getline(cmd_input, title);
  boost::algorithm::trim(title);

  if (title.empty()) {
    throw std::logic_error("Empty title is not allowed"s);
  }

  std::string author_id_par, author_name_par;
  if (auto author_name = EnterAuthorName(
          "Enter author name or empty line to select from list:"s)) {
    if (auto author = GetAuthorByName(*author_name)) {
      author_id_par = author->id;
    } else {
      if (OfferToAddAuthor(*author_name)) {
        author_name_par = *author_name;
      } else {
        throw std::runtime_error(""s);
      }
    }
  } else {
    auto author_id = SelectAuthor();
    if (not author_id.has_value())
      return std::nullopt;
    else {
      author_id_par = *author_id;
    }
  }

  auto tags = EnterBookTags("Enter tags (comma separated):");

  if (author_name_par.empty() && !author_id_par.empty()) {
    return detail::AddBookParamsWithAuthorId{title, author_id_par, tags,
                                             pub_year};
  } else if (!author_name_par.empty() && author_id_par.empty()) {
    return detail::AddBookParamsWithAuthorName{title, author_name_par, tags,
                                               pub_year};
  }
  return std::nullopt;
}

detail::EditBookParams
View::GetBookParamsForEdit(const detail::BookInfoCompletely &book) const {
  detail::EditBookParams params;

  if (auto new_title =
          EnterTitle("Enter new title or empty line to use the current one ("s +
                     book.title + "):"s)) {
    params.title = *new_title;
  }

  if (auto new_pub_year = EnterPubYear(
          "Enter publication year or empty line to use the current one ("s +
          std::to_string(book.publication_year) + "):"s)) {
    params.publication_year = *new_pub_year;
  }

  params.tags = EnterBookTags("Enter tags (current tags: "s +
                              detail::GetTagsPrintable(book.tags) + "):"s);

  return params;
}

std::optional<std::string>
View::EnterAuthorName(const std::string &introductory_phrase) const {
  output_ << introductory_phrase << std::endl;

  std::string name;
  if (!std::getline(input_, name) || name.empty()) {
    return std::nullopt;
  }

  return name;
}

bool View::OfferToAddAuthor(const std::string &author_name) const {
  output_ << "No author found. Do you want to add " << author_name << " (y/n)?"
          << std::endl;

  std::string answer;
  if (!std::getline(input_, answer) || (answer != "y" && answer != "Y")) {
    return false;
  }
  return true;
}

std::optional<std::string> View::SelectAuthor() const {
  output_ << "Select author:" << std::endl;
  auto authors = GetAuthors();
  PrintVector(output_, authors);
  output_ << "Enter author # or empty line to cancel" << std::endl;

  std::string str;
  if (!std::getline(input_, str) || str.empty()) {
    return std::nullopt;
  }

  int author_idx;
  try {
    author_idx = std::stoi(str);
  } catch (std::exception const &) {
    throw std::runtime_error("Invalid author num");
  }

  --author_idx;
  if (author_idx < 0 or author_idx >= authors.size()) {
    throw std::runtime_error("Invalid author num");
  }

  return authors[author_idx].id;
}

std::vector<std::string>
View::EnterBookTags(const std::string &introductory_phrase) const {
  output_ << introductory_phrase << std::endl;

  std::string str;
  if (!std::getline(input_, str) || str.empty()) {
    return {};
  }

  std::vector<std::string> tags;
  boost::split(tags, str, boost::is_any_of(","));

  auto is_empty = [](const std::string &s) {
    return s.find_first_not_of(" ") == std::string::npos;
  };
  std::erase_if(tags, is_empty);

  for (auto &tag : tags) {
    std::istringstream buffer(tag);
    std::vector<std::string> words{std::istream_iterator<std::string>(buffer),
                                   std::istream_iterator<std::string>()};
    tag = boost::algorithm::join(words, " "s);
  }

  std::sort(tags.begin(), tags.end());
  tags.erase(std::unique(tags.begin(), tags.end()), tags.end());

  return tags;
}

std::vector<detail::AuthorInfo> View::GetAuthors() const {
  const auto &authors = use_cases_.ShowAuthors();

  std::vector<detail::AuthorInfo> dst_authors;
  dst_authors.reserve(authors.size());

  std::transform(authors.begin(), authors.end(),
                 std::inserter(dst_authors, dst_authors.end()),
                 [](const domain::Author &author) -> detail::AuthorInfo {
                   return {author.GetId().ToString(), author.GetName()};
                 });

  return dst_authors;
}

std::vector<detail::BookInfoWithAuthor> View::GetBooks() const {
  const auto &books = use_cases_.ShowBooks();

  std::vector<detail::BookInfoWithAuthor> dst_books;
  dst_books.reserve(books.size());

  std::transform(
      books.begin(), books.end(), std::inserter(dst_books, dst_books.end()),
      [this](const domain::Book &book) -> detail::BookInfoWithAuthor {
        return {book.GetId().ToString(), book.GetTitle(), *book.GetAuthorName(),
                book.GetPubYear()};
      });

  return dst_books;
}

std::vector<detail::BookInfoByAuthor>
View::GetAuthorBooks(const std::string &author_id) const {
  const auto &authors_books = use_cases_.ShowBooksByAuthorId(author_id);

  std::vector<detail::BookInfoByAuthor> dst_books;
  dst_books.reserve(authors_books.size());

  std::transform(authors_books.begin(), authors_books.end(),
                 std::inserter(dst_books, dst_books.end()),
                 [](const domain::Book &book) -> detail::BookInfoByAuthor {
                   return {book.GetTitle(), book.GetPubYear()};
                 });

  return dst_books;
}

std::optional<detail::AuthorInfo>
View::GetAuthorByName(const std::string &name) const {
  if (auto author = use_cases_.ShowAuthorByName(name)) {
    return detail::AuthorInfo{author->GetId().ToString(), author->GetName()};
  }
  return std::nullopt;
}

std::optional<std::string>
View::SelectBook(const std::vector<detail::BookInfoWithAuthor> &books) const {
  PrintVector(output_, books);
  output_ << "Enter book # or empty line to cancel" << std::endl;

  std::string str;
  if (!std::getline(input_, str) || str.empty()) {
    return std::nullopt;
  }

  int book_idx;
  try {
    book_idx = std::stoi(str);
  } catch (std::exception const &) {
    throw std::runtime_error("Invalid book num");
  }

  --book_idx;
  if (book_idx < 0 or book_idx >= books.size()) {
    throw std::runtime_error("Invalid book num");
  }

  return books[book_idx].id;
}

std::optional<detail::BookInfoCompletely>
View::GetBookById(const std::string &book_id) const {
  if (const auto book = use_cases_.ShowBookById(book_id)) {
    return detail::BookInfoCompletely{book->GetId().ToString(),
                                      book->GetTitle(), *book->GetAuthorName(),
                                      book->GetPubYear(), book->GetTags()};
  }
  return std::nullopt;
}

std::vector<detail::BookInfoWithAuthor>
View::GetBooksByTitle(const std::string &title) const {
  const auto &books = use_cases_.ShowBooksByTitle(title);

  std::vector<detail::BookInfoWithAuthor> dst_books;
  dst_books.reserve(books.size());

  std::transform(
      books.begin(), books.end(), std::inserter(dst_books, dst_books.end()),
      [this](const domain::Book &book) -> detail::BookInfoWithAuthor {
        const auto author =
            use_cases_.ShowAuthorById(book.GetAuthorId().ToString());
        return {book.GetId().ToString(), book.GetTitle(), author->GetName(),
                book.GetPubYear()};
      });

  return dst_books;
}

std::optional<std::string>
View::EnterTitle(const std::string &introductory_phrase) const {
  output_ << introductory_phrase << std::endl;

  std::string title;
  std::getline(input_, title);
  boost::algorithm::trim(title);

  if (title.empty()) {
    return std::nullopt;
  }

  return title;
}

std::optional<int>
View::EnterPubYear(const std::string &introductory_phrase) const {
  output_ << introductory_phrase << std::endl;

  std::string str;
  if (!std::getline(input_, str) || str.empty()) {
    return std::nullopt;
  }

  int pub_year;
  try {
    pub_year = std::stoi(str);
  } catch (std::exception const &) {
    throw std::runtime_error("Invalid publication year");
  }

  return pub_year;
}

} // namespace ui