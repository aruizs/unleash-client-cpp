#include "unleash/strategies/default.h"

namespace unleash {
Default::Default(std::string_view parameters) : Strategy("default", parameters) {}

bool Default::isEnabled(const Context &context) { return true; }
}  // namespace unleash