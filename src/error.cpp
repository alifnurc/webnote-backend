#include "error.h"

namespace wnt {
std::string printError(const ErrorCode e) {
  switch (e) {
  case ErrorCode::OK:
    return "OK";
  case ErrorCode::USERNAME_NOT_FOUND:
    return "Username not found";
  case ErrorCode::INTERNAL_ERROR:
    return "Internal error";
  case ErrorCode::AUTHENTICATION_ERROR:
    return "Authentication error";
  default:
    return "Unknown error";
  }
}
} // namespace wnt
