#include "book.h"

namespace domain {

Book::Book(BookId id, AuthorId author_id, std::string title, int year)
    : id_{std::move(id)}, author_id_{std::move(author_id)},
      title_{std::move(title)}, year_{year}, author_name_{std::nullopt},
      tags_{} {}

Book::Book(BookId id, AuthorId author_id, std::string title, int year,
           const std::string &author_name)
    : id_{std::move(id)}, author_id_{std::move(author_id)},
      title_{std::move(title)}, year_{year},
      author_name_{std::move(author_name)}, tags_{} {}

const BookId &Book::GetId() const noexcept { return id_; }

const AuthorId &Book::GetAuthorId() const noexcept { return author_id_; }

const std::string &Book::GetTitle() const noexcept { return title_; }

int Book::GetPubYear() const noexcept { return year_; }

const std::vector<std::string> &Book::GetTags() const noexcept { return tags_; }

void Book::SetTags(const std::vector<std::string> &tags) {
  tags_ = std::move(tags);
}

std::optional<std::string> Book::GetAuthorName() const noexcept {
  return author_name_;
}

} // namespace domain