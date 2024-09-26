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
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/kazuho-picojson/defaults.h>
#include <string>
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
