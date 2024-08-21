#pragma once

#include <cstdint>
#include <string>
namespace wnt {
enum class ErrorCode : uint8_t {
  OK = 0,
  USERNAME_NOT_FOUND,
  INTERNAL_ERROR,
  AUTHENTICATION_ERROR
};

std::string printError(const ErrorCode e);
} // namespace wnt
