#include "unleash/strategies/gradualrolloutuserid.h"
#include "unleash/strategies/murmur3hash.h"
#include <nlohmann/json.hpp>

namespace unleash {
GradualRolloutUserId::GradualRolloutUserId(std::string_view parameters, std::string_view constraints) : Strategy("flexibleRollout", parameters, constraints) {
    auto gradualRollout_json = nlohmann::json::parse(parameters);
    m_groupId = gradualRollout_json["groupId"].get<std::string>();
    if (gradualRollout_json["percentage"].type() == nlohmann::json::value_t::string)
        m_percentage = std::stoi(gradualRollout_json["percentage"].get<std::string>());
    else
        m_percentage = gradualRollout_json["percentage"];
}

bool GradualRolloutUserId::isEnabled(const Context &context) {
    if (context.userId.empty())
        return false;
    return normalizedMurmur3(m_groupId + ":" + context.userId) <= m_percentage;
}
}  // namespace unleash