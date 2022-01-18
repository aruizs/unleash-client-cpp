#include "unleash/strategies/gradualrolloutsessionid.h"
#include "unleash/strategies/murmur3hash.h"
#include <nlohmann/json.hpp>

namespace unleash {
GradualRolloutSessionId::GradualRolloutSessionId(const std::string &parameters) : Strategy("flexibleRollout", parameters) {
    auto flexibleRollout_json = nlohmann::json::parse(parameters);
    m_groupId = flexibleRollout_json["groupId"].get<std::string>();
    m_percentage = std::stoi(flexibleRollout_json["percentage"].get<std::string>());
}

bool GradualRolloutSessionId::isEnabled(const Context &context) {
    return normalizedMurmur3(m_groupId + context.sessionId) <= m_percentage;
}
}  // namespace unleash