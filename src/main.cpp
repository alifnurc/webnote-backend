#include "db.h"

#include <chrono>
#include <crow.h>
#include <crow/app.h>
#include <crow/common.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/json.h>
#include <crow/logging.h>
#include <crow/middlewares/cors.h>
#include <crow/multipart.h>
#include <crow/query_string.h>
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/kazuho-picojson/defaults.h>
#include <regex>
#include <string>
#include <utility>
#include <variant>

// References:
// https://crowcpp.org/master/guides/middleware
struct logRequest {
  struct context {};

  // called before the handle.
  void before_handle(crow::request &req, crow::response &res, context &ctx) {
    CROW_LOG_DEBUG << "Before request handle: " + req.url;
  }

  // called after the handle.
  void after_handle(crow::request &req, crow::response &res, context &ctx) {
    CROW_LOG_DEBUG << "After request handle: " + req.url;
  }
};

// Authorization header validation.
static bool isHeaderVerified(const crow::request &req, std::string &username);

// Get message from multipart/form-data.
static bool isStringPresent(const crow::multipart::message &messages,
                            const char *key, std::string &part_message);

int main(int argc, char *argv[]) {
  // Define app and use middleware.
  crow::App<logRequest, crow::CORSHandler> app;

  // Customize CORS.
  auto &cors = app.get_middleware<crow::CORSHandler>();

  // References:
  // https://crowcpp.org/1.0/guides/CORS
  cors.global()
      .methods(crow::HTTPMethod::POST, crow::HTTPMethod::GET)
      .prefix("/")
      .origin("http://localhost:8080")
      .allow_credentials();

  CROW_ROUTE(app, "/signup")
      .methods(crow::HTTPMethod::POST)([](const crow::request &req) {
        crow::multipart::message messages(req);

        wnt::User user;
        std::string password;

        // form fields.
        if (!isStringPresent(messages, "username", user.username)) {
          return crow::response(
              crow::BAD_REQUEST,
              "Required field is missing or empty: 'username'");
        }
        if (!isStringPresent(messages, "password", password)) {
          return crow::response(
              crow::BAD_REQUEST,
              "Required field is missing or empty: 'password'");
        }

        if (!wnt::create_new_account(user, password)) {
          return crow::response(
              crow::INTERNAL_SERVER_ERROR,
              wnt::printError(wnt::ErrorCode::INTERNAL_ERROR));
        }

        return crow::response(200, "OK");
      });

  CROW_ROUTE(app, "/signin")
      .methods(crow::HTTPMethod::POST)([](const crow::request &req) {
        // Request body validation.
        const auto &headers = req.headers.find("Content-Length");
        if (headers == req.headers.end()) {
          return crow::response(crow::BAD_REQUEST,
                                "Missing Content-Length header");
        }

        // Request body length validation.
        if (req.body.size() <= 23) {
          return crow::response(crow::status::BAD_REQUEST,
                                "Request body too short");
        }

        // Request body json validation.
        crow::json::rvalue body_json =
            crow::json::load(req.body.c_str(), req.body.size());
        if (body_json.error()) {
          return crow::response(crow::status::BAD_REQUEST,
                                "Request body is not JSON");
        }

        // Request body fields validation.
        if (!body_json.has("username") || !body_json.has("password") ||
            body_json["username"].s().size() == 0 ||
            body_json["password"].s().size() == 0) {
          return crow::response(crow::status::BAD_REQUEST,
                                "Missing username and/or password");
        }

        // Get credentials from request body.
        const std::string username = body_json["username"].s();
        const std::string password = body_json["password"].s();

        // Check username if exists.
        auto result = wnt::get_user(username);
        if (std::holds_alternative<wnt::ErrorCode>(result)) {
          return crow::response(
              crow::status::UNAUTHORIZED,
              wnt::printError(std::get<wnt::ErrorCode>(result)));
        }

        // Check password if valid.
        const auto &user = std::get<wnt::User>(result);
        if (!wnt::is_valid_password(password, user.password)) {
          return crow::response(
              crow::status::UNAUTHORIZED,
              wnt::printError(wnt::ErrorCode::AUTHENTICATION_ERROR));
        }

        // Create a token for user authentication, contains user data and is
        // valid for one hour.
        auto current_time = std::chrono::system_clock::now();
        auto token =
            jwt::create()
                .set_issuer("WNT")
                .set_type("JWS")
                .set_issued_at(current_time)
                .set_expires_at(current_time + std::chrono::seconds{3600})
                .set_payload_claim("username", jwt::claim(user.username))
                .sign(jwt::algorithm::hs512{"secret"});

        // Prepare response with access token and username for client side.
        crow::json::wvalue response{{"access_token", token},
                                    {"username", user.username}};
        return crow::response(crow::status::OK, response);
      });

  CROW_ROUTE(app, "/addnote")
      .methods(crow::HTTPMethod::POST)([](const crow::request &req) {
        wnt::Note note;
        // verify authorization header.
        if (!isHeaderVerified(req, note.username)) {
          return crow::response(
              crow::status::UNAUTHORIZED,
              wnt::printError(wnt::ErrorCode::AUTHENTICATION_ERROR));
        }

        crow::multipart::message messages(req);
        /*return crow::response(crow::status::OK);*/

        // Form fields validation.
        if (!isStringPresent(messages, "note_title", note.title)) {
          return crow::response(
              crow::status::BAD_REQUEST,
              "Required field is missing or empty: 'title note'");
        }
        if (!isStringPresent(messages, "note_description", note.description)) {
          return crow::response(
              crow::status::BAD_REQUEST,
              "Required field is missing or empty: 'description note'");
        }

        if (!wnt::add_note(note)) {
          return crow::response(
              crow::status::INTERNAL_SERVER_ERROR,
              wnt::printError(wnt::ErrorCode::INTERNAL_ERROR));
        }
        return crow::response(crow::status::OK);
      });

  CROW_ROUTE(app, "/listnotes")
      .methods(crow::HTTPMethod::GET)([](const crow::request &req) {
        std::string username;
        if (!isHeaderVerified(req, username)) {
          return crow::response(
              crow::status::UNAUTHORIZED,
              wnt::printError(wnt::ErrorCode::AUTHENTICATION_ERROR));
        }

        // Request query string validation.
        const crow::query_string &q = req.url_params;
        uint32_t page_size = std::stoi(q.get("page_size"));
        uint32_t current_page = std::stoi(q.get("current_page"));
        std::optional<std::string> search =
            q.get("search") == nullptr ? std::nullopt
                                       : std::make_optional(q.get("search"));
        std::optional<std::string> sort_by =
            q.get("sort_by") == nullptr ? std::nullopt
                                        : std::make_optional(q.get("sort_by"));

        // Get result of query.
        auto result =
            wnt::get_notes_list(username, page_size, current_page,
                                std::move(search), std::move(sort_by));
        if (std::holds_alternative<wnt::ErrorCode>(result)) {
          wnt::ErrorCode ecode = std::get<wnt::ErrorCode>(result);
          return crow::response(crow::status::INTERNAL_SERVER_ERROR,
                                wnt::printError(ecode));
        }
        const auto &notes = std::get<std::vector<wnt::Note>>(result);
        std::vector<crow::json::wvalue> notes_json_list;

        // Convert notes to json list.
        for (const auto &note : notes) {
          crow::json::wvalue note_json{
              {"id", note.id},
              {"username", note.username},
              {"title", note.title},
              {"description", note.description},
              {"creation_date", note.creation_date},
              {"last_update_date", note.last_update_date},
          };
          notes_json_list.push_back(note_json);
        }
        return crow::response(crow::status::OK, crow::json::wvalue({
                                                    {"notes", notes_json_list},
                                                }));
      });

  CROW_ROUTE(app, "/updatenote")
      .methods(crow::HTTPMethod::POST)([](const crow::request &req) {
        wnt::Note note;

        if (!isHeaderVerified(req, note.username)) {
          return crow::response(
              crow::status::UNAUTHORIZED,
              wnt::printError(wnt::ErrorCode::AUTHENTICATION_ERROR));
        }

        const crow::query_string &q = req.url_params;
        const char *note_id = q.get("note_id");

        if (note_id == nullptr) {
          return crow::response(crow::status::BAD_REQUEST,
                                "id parameter is missing or empty");
        }

        note.id = std::stoull(note_id);
        crow::multipart::message messages(req);

        // Form fields validation.
        isStringPresent(messages, "note_title", note.title);
        isStringPresent(messages, "note_description", note.description);

        if (!wnt::update_note(note)) {
          return crow::response(
              crow::status::INTERNAL_SERVER_ERROR,
              wnt::printError(wnt::ErrorCode::INTERNAL_ERROR));
        }
        return crow::response(crow::status::OK);
      });

  CROW_ROUTE(app, "/deletenote")
      .methods(crow::HTTPMethod::POST)([](const crow::request &req) {
        std::string username;
        if (!isHeaderVerified(req, username)) {
          return crow::response(
              crow::status::UNAUTHORIZED,
              wnt::printError(wnt::ErrorCode::AUTHENTICATION_ERROR));
        }

        const crow::query_string &q = req.url_params;
        const char *note_id = q.get("note_id");

        if (note_id == nullptr) {
          return crow::response(crow::status::BAD_REQUEST,
                                "id parameter is missing or empty");
        }

        uint64_t id = std::stoull(note_id);

        if (!wnt::delete_note(username, id)) {
          return crow::response(
              crow::status::INTERNAL_SERVER_ERROR,
              wnt::printError(wnt::ErrorCode::INTERNAL_ERROR));
        }
        return crow::response(crow::status::OK);
      });

  // Log level is set to DEBUG.
  app.loglevel(crow::LogLevel::DEBUG);

  // Set up port, set the app to run in multithread and run the app.
  app.bindaddr("127.0.0.1").port(5000).multithreaded().run();
  return 0;
}

// Reference:
// https://crowcpp.org/1.0/guides/multipart
static bool isStringPresent(const crow::multipart::message &messages,
                            const char *key, std::string &part_message) {
  auto i = messages.part_map.find(key);
  if (i == messages.part_map.end() || i->second.body.empty()) {
    return false;
  }
  part_message = i->second.body;
  return true;
}

// Verify header and extract username.
static bool isHeaderVerified(const crow::request &req, std::string &username) {
  // Try to find Authorization header in request.
  const auto &headers = req.headers.find("Authorization");
  if (headers == req.headers.end()) {
    CROW_LOG_ERROR << "Request header does not contain Authorization";
    return false;
  }

  // Extract the value of Authorization header.
  const std::string &authorization_value = headers->second;
  // This pattern expects "Bearer <token>", where <token> is a string of allowed
  // characters.
  std::regex bearer_scheme_regex("Bearer +([A-Za-z0-9_\\-.~+]+[=]*)");

  // Attempt to match the Authorization header value with the bearer token
  // pattern.
  std::smatch match;
  if (!std::regex_match(authorization_value, match, bearer_scheme_regex)) {
    CROW_LOG_ERROR << "Request header Authorization does not contain Bearer";
    return false;
  }

  // Decode the Bearer token (found in the regex match).
  auto decade = jwt::decode(match[1].str());
  // Set up a JWT verifier.
  // The algorithm is set to HS512 and the issuer is set to WNT.
  auto verifier = jwt::verify()
                      .allow_algorithm(jwt::algorithm::hs512{"secret"})
                      .with_issuer("WNT");

  // Try to verify the token.
  // If the token is not valid, an exception will be thrown.
  try {
    verifier.verify(decade);
  } catch (const std::exception &e) {
    CROW_LOG_ERROR << "Failed to verify token: " << e.what();
    return false;
  }

  // If the token is valid, extract the username from the token.
  username = decade.get_payload_claim("username").as_string();
  return true;
}
