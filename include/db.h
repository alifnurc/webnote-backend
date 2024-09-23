#pragma once

#include "error.h"
#include "note.h"
#include "user.h"
#include <variant>
namespace wnt {
bool create_new_account(const User &user, const std::string &password);
bool is_valid_password(const std::string &password_input,
                       const std::string &stored_password);
std::variant<User, ErrorCode> get_user(const std::string &username);
} // namespace wnt
