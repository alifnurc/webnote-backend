#pragma once

#include <cstdint>
#include <string>
namespace wnt {
struct Note {
  uint64_t id;
  std::string username;
  std::string title;
  std::string description;
  std::string creation_date;
  std::string last_update_date;
};
} // namespace wnt
