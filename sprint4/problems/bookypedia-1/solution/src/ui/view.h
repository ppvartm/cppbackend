#pragma once
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>


#include"../domain/author.h"
namespace menu {
class Menu;
}

namespace app {
class UseCases;
}

namespace ui {
namespace detail {

struct AddBookParams {
    std::string title;
    std::string author_id;
    int publication_year = 0;
};


struct AuthorInfo {
    std::string id;
    std::string name;
    std::string uuid;
};

struct BookInfo {
    std::string title;
    int publication_year;
    
    BookInfo() {};
    BookInfo(const BookInfo&) = default;
    BookInfo(std::pair<std::string, uint16_t> p): title(p.first), publication_year(p.second){}
    BookInfo(std::string tit, int pub_year):title(tit), publication_year(pub_year){}
    BookInfo& operator=(std::pair<std::string, uint16_t> p) {
        title = p.first;
        publication_year = p.second;
        return *this;
    }
};

}  // namespace detail

class View {
public:
    View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output);

private:
    bool AddAuthor(std::istream& cmd_input) const;
    bool AddBook(std::istream& cmd_input) const;
    bool ShowAuthors() const;
    bool ShowBooks() const;
    bool ShowAuthorBooks() const;

    std::optional<detail::AddBookParams> GetBookParams(std::istream& cmd_input) const;
    std::optional<std::string> SelectAuthor() const;
    std::vector<detail::AuthorInfo> GetAuthors() const;
    std::vector<detail::AuthorInfo> GetFullInfoAboutAuthors() const;
    std::vector<detail::BookInfo> GetBooks() const;
    std::vector<detail::BookInfo> GetAuthorBooks(const std::string& author_id) const;

    menu::Menu& menu_;
    app::UseCases& use_cases_;
    std::istream& input_;
    std::ostream& output_;
};

}  // namespace ui