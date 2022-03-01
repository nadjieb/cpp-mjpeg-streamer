#pragma once

#include <iostream>

namespace nadjieb {
static void logError(const std::string& error_message) {
    std::cerr << error_message;
}
}  // namespace nadjieb
