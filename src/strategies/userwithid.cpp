#include "unleash/strategies/userwithid.h"
#include <nlohmann/json.hpp>
#include <sstream>

namespace unleash {
UserWithId::UserWithId(const std::string &parameters) : Strategy("userWithId", parameters) {
    auto usersId_json = nlohmann::json::parse(parameters);
    const std::string delimiter = ",";
    std::stringstream sstream(usersId_json["userIds"].get<std::string>());
    std::string userId;
    while (std::getline(sstream, userId, ',')) {
        m_userIds.push_back(userId);
    }
}

bool UserWithId::isEnabled(const Context &context) {
    if (std::find(m_userIds.begin(), m_userIds.end(), context.userId) != m_userIds.end())
        return true;
    return false;
}
}  // namespace unleash