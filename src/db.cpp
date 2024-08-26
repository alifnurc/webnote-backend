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
      << transaction.quote(user.username) << ',' << transaction.quote(password)
      << ',' << transaction.quote(user.account_birth)
      << ", generate_salt('bf', )), now())";
    const std::string query = s.str();
    CROW_LOG_DEBUG << "Query: " << query;
    auto result = transaction.exec(query);
    if (result.affected_rows() != 1) {
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
} // namespace wnt