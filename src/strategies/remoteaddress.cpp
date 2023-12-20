#include "unleash/strategies/remoteaddress.h"
#include <nlohmann/json.hpp>
#include <sstream>

namespace unleash {
RemoteAddress::RemoteAddress(std::string_view parameters, std::string_view constraints) : Strategy("remoteAddress", constraints) {
    auto remoteAddress_json = nlohmann::json::parse(parameters);
    const std::string delimiter = ",";
    std::stringstream sstream(remoteAddress_json["IPs"].get<std::string>());
    std::string ip;
    while (std::getline(sstream, ip, ',')) {
        ip.erase(remove(ip.begin(), ip.end(), ' '), ip.end());
        m_ips.push_back(ip);
    }
}

bool RemoteAddress::isEnabled(const Context &context) {
    if (std::find(m_ips.begin(), m_ips.end(), context.remoteAddress) != m_ips.end())
        return true;
    return false;
}
}  // namespace unleash