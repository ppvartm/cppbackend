#pragma once

#include "book.h"
#include <optional>

namespace domain {

class BookTag {
public:
  explicit BookTag(BookId book_id, const std::string &tag);

  const BookId &GetBookId() const noexcept;
  const std::string &GetTag() const noexcept;

private:
  BookId book_id_;
  std::string tag_;
};

class BookTagRepository {
public:
  virtual void Save(const BookTag &book_tag) = 0;
  virtual std::vector<BookTag> GetAllByBookId(const BookId &book_id) const = 0;
  virtual void DeleteByBookId(const BookId &book_id) const = 0;

protected:
  ~BookTagRepository() = default;
};

} // namespace domain