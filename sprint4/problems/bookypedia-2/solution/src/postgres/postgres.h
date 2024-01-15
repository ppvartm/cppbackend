#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>
#include <vector>

#include "../domain/author.h"
#include "../domain/book.h"
#include "../domain/book_tag.h"

#include "../app/unit_of_work.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
  explicit AuthorRepositoryImpl(pqxx::work &work);

  void Save(const domain::Author &author) override;
  std::vector<domain::Author> GetAll() const override;
  std::optional<domain::Author>
  LoadByName(const std::string &name) const override;
  std::optional<domain::Author>
  LoadById(const domain::AuthorId &id) const override;
  void Delete(const domain::AuthorId &id) override;
  void Edit(const domain::AuthorId &id, const std::string &name) override;

private:
  std::vector<domain::Author>
  GetAuthorsFromResult(const pqxx::result &result) const;

  domain::Author GetAuthorFromRow(const pqxx::row &row) const;

private:
  pqxx::work &work_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
  explicit BookRepositoryImpl(pqxx::work &work);

  void Save(const domain::Book &book) override;
  std::vector<domain::Book> GetAll() const override;

  std::vector<domain::Book>
  GetAllByAuthor(const domain::AuthorId &author_id) const override;

  std::optional<domain::Book>
  LoadById(const domain::BookId &book_id) const override;

  std::vector<domain::Book>
  LoadByTitle(const std::string &title) const override;

  void Delete(const domain::BookId &id) override;

  void Edit(const domain::BookId &id, const std::string &title,
            int pub_year) override;

  void DeleteBooksByAuthorId(const domain::AuthorId &author_id) override;

private:
  struct GetResultConfig {
    bool with_author_name;
  };

  std::vector<domain::Book>
  GetBooksFromResult(const pqxx::result &result,
                     GetResultConfig config = GetResultConfig{false}) const;

  domain::Book GetBookFromRow(const pqxx::row &row,
                              GetResultConfig config) const;

private:
  pqxx::work &work_;
};

class BookTagRepositoryImpl : public domain::BookTagRepository {
public:
  explicit BookTagRepositoryImpl(pqxx::work &work);

  void Save(const domain::BookTag &book_tag) override;

  std::vector<domain::BookTag>
  GetAllByBookId(const domain::BookId &book_id) const override;

  void DeleteByBookId(const domain::BookId &book_id) const override;

private:
  std::vector<domain::BookTag>
  GetBookTagsFromResult(const pqxx::result &result) const;

  domain::BookTag GetBookTagFromRow(const pqxx::row &row) const;

private:
  pqxx::work &work_;
};

class UnitOfWorkImpl : public app::UnitOfWork {
public:
  explicit UnitOfWorkImpl(pqxx::connection &connection);

  void Commit() override;

  AuthorRepositoryImpl &Authors() override;
  BookRepositoryImpl &Books() override;
  BookTagRepositoryImpl &BookTags() override;

private:
  pqxx::work work_;
  AuthorRepositoryImpl authors_{work_};
  BookRepositoryImpl books_{work_};
  BookTagRepositoryImpl book_tags_{work_};
};

class UOWFactoryImpl : public app::UnitOfWorkFactory {
public:
  explicit UOWFactoryImpl(pqxx::connection &connection);

  std::unique_ptr<app::UnitOfWork> CreateUnitOfWork() override;

private:
  pqxx::connection &connection_;
};

class Database {
public:
  explicit Database(pqxx::connection connection);

  UOWFactoryImpl &GetUOWFactory();

private:
  pqxx::connection connection_;
  UOWFactoryImpl uow_factory_{connection_};
};

} // namespace postgres