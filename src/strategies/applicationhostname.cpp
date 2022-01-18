#include "unleash/strategies/applicationhostname.h"
#include <nlohmann/json.hpp>

namespace unleash {
ApplicationHostname::ApplicationHostname(const std::string &parameters) : Strategy("applicationHostname", parameters) {
    auto applicationHostname_json = nlohmann::json::parse(parameters);
    const std::string delimiter = ",";
    std::stringstream sstream(applicationHostname_json["applicationHostnames"].get<std::string>());
    std::string userId;
    while (std::getline(sstream, userId, ',')) {
        m_applicationHostnames.push_back(userId);
    }
}

bool ApplicationHostname::isEnabled(const Context &context) {
    if (std::find(m_applicationHostnames.begin(), m_applicationHostnames.end(), context.userId) != m_applicationHostnames.end())
        return true;
    return false;
}
}  // namespace unleash