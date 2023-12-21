#include "unleash/strategies/default.h"

namespace unleash {
Default::Default(std::string_view, std::string_view constraints) : Strategy("default", constraints) {}

bool Default::isEnabled(const Context &context) { return meetConstraints(context); }
}  // namespace unleash