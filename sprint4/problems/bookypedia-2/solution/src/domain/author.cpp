#include "author.h"

namespace domain {

Author::Author(AuthorId id, std::string name)
    : id_(std::move(id)), name_(std::move(name)) {}

const AuthorId &Author::GetId() const noexcept { return id_; }

const std::string &Author::GetName() const noexcept { return name_; }

} // namespace domain
