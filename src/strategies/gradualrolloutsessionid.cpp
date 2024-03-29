#include "unleash/strategies/gradualrolloutsessionid.h"
#include "unleash/utils/murmur3hash.h"
#include <nlohmann/json.hpp>

namespace unleash {
GradualRolloutSessionId::GradualRolloutSessionId(std::string_view parameters, std::string_view constraints)
    : Strategy("flexibleRollout", constraints) {
    auto gradualRollout_json = nlohmann::json::parse(parameters);
    m_groupId = gradualRollout_json["groupId"].get<std::string>();
    m_percentage = (gradualRollout_json["percentage"].type() == nlohmann::json::value_t::string)
                           ? std::stoi(gradualRollout_json["percentage"].get<std::string>())
                           : uint32_t(gradualRollout_json["percentage"]);
}

bool GradualRolloutSessionId::isEnabled(const Context &context) {
    if (context.sessionId.empty()) return false;
    return normalizedMurmur3(m_groupId + ":" + context.sessionId) <= m_percentage;
}
}  // namespace unleash