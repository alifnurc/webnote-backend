#include "db.h"

#include <crow.h>
#include <crow/app.h>
#include <crow/common.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/logging.h>
#include <crow/middlewares/cors.h>
#include <crow/multipart.h>
#include <string>

// References:
// https://crowcpp.org/master/guides/middleware
struct logRequest {
  struct context {};

  // called before the handle.
  void beforeHandle(crow::request &req, crow::response &res, context &ctx) {
    CROW_LOG_DEBUG << "Before request handle: " + req.url;
  }

  // called after the handle.
  void afterHandle(crow::request &req, crow::response &res, context &ctx) {
    CROW_LOG_DEBUG << "After request handle: " + req.url;
  }
};

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
  return 0;
}
