#include "unleash/strategies/default.h"

namespace unleash {
Default::Default(const std::string &parameters) : Strategy("default", parameters) {}

bool Default::isEnabled(const Context &context) { return true; }
}  // namespace unleash