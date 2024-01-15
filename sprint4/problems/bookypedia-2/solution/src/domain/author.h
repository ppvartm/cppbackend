#pragma once
#include <optional>
#include <string>
#include <vector>

#include "../util/tagged_uuid.h"

namespace domain {

namespace detail {
struct AuthorTag {};
} // namespace detail

using AuthorId = util::TaggedUUID<detail::AuthorTag>;

class Author {
public:
  Author(AuthorId id, std::string name);

  const AuthorId &GetId() const noexcept;

  const std::string &GetName() const noexcept;

private:
  AuthorId id_;
  std::string name_;
};

class AuthorRepository {
public:
  virtual void Save(const Author &author) = 0;
  virtual std::vector<Author> GetAll() const = 0;
  virtual std::optional<Author> LoadByName(const std::string &name) const = 0;
  virtual std::optional<Author> LoadById(const AuthorId &id) const = 0;
  virtual void Delete(const AuthorId &id) = 0;
  virtual void Edit(const AuthorId &id, const std::string &name) = 0;

protected:
  ~AuthorRepository() = default;
};

} // namespace domain
