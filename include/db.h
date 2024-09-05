#pragma once

#include "error.h"
#include "note.h"
#include "user.h"
namespace wnt {
bool create_new_account(const User &user, const std::string &password);
bool is_valid_password(const std::string &password_input,
                       const std::string &stored_password);
} // namespace wnt
