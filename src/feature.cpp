#include "unleash/feature.h"

namespace unleash {
Feature::Feature(std::string name, std::vector<std::unique_ptr<Strategy>> strategies, bool enable) : m_name(std::move(name)), m_strategies(std::move(strategies)), m_enabled(enable) {}
bool Feature::isEnabled(const Context &context) const {
    if (m_enabled) {
        if (m_strategies.empty())
            return true;
        for (const auto &strategy : m_strategies) {
            if (strategy->isEnabled(context)) {
                return true;
            }
        }
    }
    return false;
}
}  // namespace unleash