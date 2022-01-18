#include "unleash/strategies/remoteaddress.h"
#include <nlohmann/json.hpp>

namespace unleash {
RemoteAddress::RemoteAddress(const std::string &parameters) : Strategy("remoteAddress", parameters) {
    auto remoteAddres_json = nlohmann::json::parse(parameters);
    const std::string delimiter = ",";
    std::stringstream sstream(remoteAddres_json["IPs"].get<std::string>());
    std::string ip;
    while (std::getline(sstream, ip, ',')) {
        m_ips.push_back(ip);
    }
}

bool RemoteAddress::isEnabled(const Context &context) {
    if (std::find(m_ips.begin(), m_ips.end(), context.userId) != m_ips.end())
        return true;
    return false;
}
}  // namespace unleash