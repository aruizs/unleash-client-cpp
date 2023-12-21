#include "unleash/strategies/userwithid.h"
#include <nlohmann/json.hpp>
#include <sstream>

namespace unleash {
UserWithId::UserWithId(std::string_view parameters, std::string_view constraints)
    : Strategy("userWithId", constraints) {
    auto usersId_json = nlohmann::json::parse(parameters);
    const std::string delimiter = ",";
    std::stringstream sstream(usersId_json["userIds"].get<std::string>());
    std::string userId;
    while (std::getline(sstream, userId, ',')) {
        userId.erase(remove(userId.begin(), userId.end(), ' '), userId.end());
        m_userIds.push_back(userId);
    }
}

bool UserWithId::isEnabled(const Context &context) {
    if (meetConstraints(context) && std::find(m_userIds.begin(), m_userIds.end(), context.userId) != m_userIds.end())
        return true;
    return false;
}
}  // namespace unleash