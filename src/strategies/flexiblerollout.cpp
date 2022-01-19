#include "unleash/strategies/flexiblerollout.h"
#include "unleash/strategies/murmur3hash.h"
#include <nlohmann/json.hpp>
#include <random>

namespace unleash {
FlexibleRollout::FlexibleRollout(const std::string &parameters) : Strategy("flexibleRollout", parameters) {
    auto flexibleRollout_json = nlohmann::json::parse(parameters);
    m_groupId = flexibleRollout_json["groupId"].get<std::string>();
    m_rollout = std::stoi(flexibleRollout_json["rollout"].get<std::string>());
    m_stickiness = flexibleRollout_json["stickiness"].get<std::string>();
}

bool FlexibleRollout::isEnabled(const Context &context) {
    auto stickinessConfiguration = m_stickiness;
    // Choose strategy configuration
    if (stickinessConfiguration == "default") {
        if (!context.userId.empty())
            stickinessConfiguration = "userId";
        else if (!context.sessionId.empty())
            stickinessConfiguration = "sessionId";
        else
            stickinessConfiguration = "random";
    }
    if (stickinessConfiguration == "userId") {
        return normalizedMurmur3(m_groupId + ":" + context.userId) <= m_rollout;
    } else if (stickinessConfiguration == "sessionId") {
        return normalizedMurmur3(m_groupId + context.sessionId) <= m_rollout;
    } else if (stickinessConfiguration == "random") {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist6(1, 100);
        return dist6(rng) <= m_rollout;
    }
    return false;
}
}  // namespace unleash