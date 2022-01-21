#include "unleash/strategies/gradualrolloutuserid.h"
#include "unleash/strategies/murmur3hash.h"
#include <nlohmann/json.hpp>
#include <iostream>
namespace unleash {
GradualRolloutUserId::GradualRolloutUserId(std::string_view parameters) : Strategy("flexibleRollout", parameters) {
    auto flexibleRollout_json = nlohmann::json::parse(parameters);
    m_groupId = flexibleRollout_json["groupId"].get<std::string>();
    m_percentage = std::stoi(flexibleRollout_json["percentage"].get<std::string>());
}

bool GradualRolloutUserId::isEnabled(const Context &context) {
    if (context.userId.empty())
        return false;
    return normalizedMurmur3(m_groupId  + ":" +  context.userId) <= m_percentage;
}
}  // namespace unleash