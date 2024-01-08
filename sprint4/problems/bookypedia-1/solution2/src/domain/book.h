#pragma once
#include <string>

#include "author.h"
#include "../util/tagged_uuid.h"

namespace domain {

    namespace detail {
        struct BookTag {};
    }  // namespace detail

    using BookId = util::TaggedUUID<detail::BookTag>;

    class Book {
    public:
        Book(BookId book_id, AuthorId author_id, std::string title, uint16_t publication_year)
            : id_(std::move(book_id)),
              author_id_(std::move(author_id)),
              title_(std::move(title)),
              publication_year_(publication_year){
        }

        const BookId& GetId() const noexcept {
            return id_;
        }
        const AuthorId& GetAuthorId() const noexcept {
            return author_id_;
        }
        const std::string& GetTitle() const noexcept {
            return title_;
        }

        uint16_t GetPublicationYear() const noexcept {
            return publication_year_;
        }

    private:
        BookId id_;
        AuthorId author_id_;
        std::string title_;
        uint16_t publication_year_;
    };

    class BookRepository {
    public:
        virtual void Save(const Book& author) = 0;

    protected:
        ~BookRepository() = default;
    };

}  // namespace domain
