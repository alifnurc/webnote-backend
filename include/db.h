#pragma once

#include "error.h"
#include "note.h"
#include "user.h"
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>
namespace wnt {
bool create_new_account(const User &user, const std::string &password);
bool is_valid_password(const std::string &password_input,
                       const std::string &stored_password);
std::variant<User, ErrorCode> get_user(const std::string &username);
bool add_note(const Note &note);
std::variant<std::vector<Note>, ErrorCode>
get_notes_list(const std::string &username, uint32_t page_size,
               uint32_t current_page, std::optional<std::string> search,
               std::optional<std::string> sort_by);
bool update_note(const Note &note);
} // namespace wnt
