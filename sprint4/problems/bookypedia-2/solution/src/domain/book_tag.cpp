#include "book_tag.h"

namespace domain {

BookTag::BookTag(BookId book_id, const std::string &tag)
    : book_id_{std::move(book_id)}, tag_{std::move(tag)} {}

const BookId &BookTag::GetBookId() const noexcept { return book_id_; }
const std::string &BookTag::GetTag() const noexcept { return tag_; }

} // namespace domain