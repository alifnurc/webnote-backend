#pragma once

#include "error.h"
#include "note.h"
#include "user.h"
namespace wnt {
bool create_new_account(const User &user, const std::string &password);
}
