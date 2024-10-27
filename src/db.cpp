#include "db.h"

#include <crow/logging.h>
#include <pqxx/pqxx>
#include <sstream>
#include <string>

const std::string HOST = "127.0.0.1";
const std::string PORT = "5432";
const std::string DBNAME = "crab";
const std::string USER = "hitagi";
const std::string PASSWORD = "hitagi";
const std::string url = "host=" + HOST + " port=" + PORT + " dbname=" + DBNAME +
                        " user=" + USER + " password=" + PASSWORD;

namespace wnt {
bool create_new_account(const User &user, const std::string &password) {
  pqxx::connection c(url);
  pqxx::work transaction(c);

  try {
    std::stringstream s;
    s << "INSERT INTO userwebnote(username, password, account_birth) "
         "VALUES("
      << transaction.quote(user.username) << ", crypt("
      << transaction.quote(password) << ", gen_salt('bf', 5)), now())";

    const std::string query = s.str();

    // Debug query.
    CROW_LOG_DEBUG << "Query: " + query;

    // Exec query and insert it to database.
    auto result = transaction.exec(query);
    if (result.affected_rows() != 1) { // number of rows affected.
      CROW_LOG_ERROR << "Could not insert user to database";
      return false;
    }

    transaction.commit();
    return true;
  } catch (const pqxx::sql_error &e) {
    CROW_LOG_ERROR << "Internal exception was thrown: " << e.what();
    return false;
  }
}

bool is_valid_password(const std::string &password_input,
                       const std::string &stored_password) {
  pqxx::connection c(url);
  pqxx::read_transaction transaction(c);

  try {
    std::stringstream s;
    s << "SELECT (CASE WHEN crypt(" << transaction.quote(password_input) << ","
      << transaction.quote(stored_password)
      << ") = " << transaction.quote(stored_password)
      << " THEN true ELSE false END) AS is_equal";

    std::string query = s.str();

    // Debug query.
    CROW_LOG_DEBUG << "Query: " + query;

    // Get result of query.
    bool is_match_password = transaction.query_value<bool>(query);

    transaction.commit();
    return is_match_password;
  } catch (const pqxx::unexpected_rows &e) {
    CROW_LOG_ERROR << "Number of rows returned is not equal to 1: " << e.what();
    return false;
  } catch (const pqxx::usage_error &e) {
    CROW_LOG_ERROR << "Number of columns returned is not equal to 1: "
                   << e.what();
    return false;
  } catch (const pqxx::sql_error &e) {
    CROW_LOG_ERROR << "Internal exception was thrown: " << e.what();
    return false;
  }
}

std::variant<User, ErrorCode> get_user(const std::string &username) {
  pqxx::connection c(url);
  pqxx::read_transaction transaction(c);

  try {
    // Search user from database.
    auto row = transaction.exec1("SELECT * FROM userwebnote WHERE username=" +
                                 transaction.quote(username));
    transaction.commit();

    return User{.username = row["username"].c_str(),
                .password = row["password"].c_str(),
                .account_birth = row["account_birth"].c_str()};
  } catch (const pqxx::unexpected_rows &e) {
    CROW_LOG_ERROR << "Number of rows returned is not equal to 1: " << e.what();
    return ErrorCode::INTERNAL_ERROR;
  }
}

bool add_note(const Note &note) {
  pqxx::connection c(url);
  pqxx::work transaction(c);

  try {
    std::stringstream s;
    s << "INSERT INTO datawebnote(username, title, description, creation_date, "
         "last_update_date) VALUES("
      << transaction.quote(note.username) << ','
      << transaction.quote(note.title) << ','
      << transaction.quote(note.description) << ", now(), now())";

    // Exec query and insert it to database.
    auto result = transaction.exec(s.str());
    if (result.affected_rows() != 1) { // number of rows affected.
      CROW_LOG_ERROR << "Could not insert note to database";
      return false;
    }

    transaction.commit();
    return true;
  } catch (const pqxx::sql_error &e) {
    CROW_LOG_ERROR << "Internal exception was thrown: " << e.what();
    return false;
  }
}

std::variant<std::vector<Note>, ErrorCode>
get_notes_list(const std::string &username, uint32_t page_size,
               uint32_t current_page, std::optional<std::string> search,
               std::optional<std::string> sort_by) {
  pqxx::connection c(url);
  pqxx::read_transaction transaction(c);

  try {
    // Search user from database.
    std::stringstream s;
    s << "SELECT * FROM datawebnote WHERE username="
      << transaction.quote(username);
    if (search.has_value()) {
      s << " AND title LIKE " << transaction.quote("%" + search.value() + "%");
    }

    std::string sort_by_val = "asc";
    if (sort_by.has_value() && sort_by.value() == "last_update_date") {
      sort_by_val = "desc";
    }

    // Pagination with offset and limit.
    s << " ORDER BY last_update_date " << sort_by_val;
    s << " OFFSET " << (page_size * (current_page - 1));
    s << " LIMIT " << page_size;

    std::string query = s.str();

    // Debug query.
    CROW_LOG_DEBUG << "Query: " + query;

    // Get result of query.
    auto result = transaction.exec(query);

    // Get data and asign it to vector.
    std::vector<Note> notes;
    for (const auto &row : result) {
      notes.push_back(
          Note{.id = row["id"].as<uint64_t>(),
               .username = row["username"].c_str(),
               .title = row["title"].c_str(),
               .description = row["description"].c_str(),
               .creation_date = row["creation_date"].c_str(),
               .last_update_date = row["last_update_date"].c_str()});
    }
    return notes;
  } catch (const pqxx::unexpected_rows &e) {
    CROW_LOG_ERROR << "Number of rows returned is not equal to 1: " << e.what();
    return ErrorCode::INTERNAL_ERROR;
  }
}

bool update_note(const Note &note) {
  pqxx::connection c(url);
  pqxx::work transaction(c);

  try {
    std::stringstream s;
    s << "UPDATE datawebnote SET ";

    if (!note.title.empty()) {
      s << "title=" << transaction.quote(note.title) << ',';
    }
    if (!note.description.empty()) {
      s << "description=" << transaction.quote(note.description) << ',';
    }

    s << "last_update_date=now() WHERE username="
      << transaction.quote(note.username) << " AND id=" << note.id;

    auto result = transaction.exec(s.str());
    if (result.affected_rows() != 1) { // number of rows affected.
      CROW_LOG_ERROR << "Could not update note to database";
      return false;
    }

    transaction.commit();
    return true;
  } catch (const pqxx::sql_error &e) {
    CROW_LOG_ERROR << "Internal exception was thrown: " << e.what();
    return false;
  }
}

bool delete_note(const std::string &username, uint16_t id) {
  pqxx::connection c(url);
  pqxx::work transaction(c);

  try {
    std::stringstream s;
    s << "DELETE FROM datawebnote WHERE username="
      << transaction.quote(username) << " AND id=" << id;

    auto result = transaction.exec(s.str());
    if (result.affected_rows() != 1) { // number of rows affected.
      CROW_LOG_ERROR << "Could not delete note to database";
      return false;
    }

    transaction.commit();
    return true;
  } catch (const pqxx::sql_error &e) {
    CROW_LOG_ERROR << "Internal exception was thrown: " << e.what();
    return false;
  }
}
} // namespace wnt
